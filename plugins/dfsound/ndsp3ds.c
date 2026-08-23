/*
 * NDSP output driver for the 3DS (PicaStation).
 *
 * dfsound hands us ~one emulated frame of 44.1kHz stereo PCM16 per
 * call; each feed lands in its own NDSP wave buffer, so the queue
 * depth IS the audio latency (kept near an adaptive target, 3-7 frames).
 *
 * Drift control: the emulator does not run at exactly 50/60Hz (the
 * LCD-locked pacer runs PAL at 49.86fps by design, and games vary),
 * so producing 44100 samples/s of audio while the DSP consumes
 * exactly 44100/s would underrun every few seconds. Instead of
 * resampling, the DSP's playback rate is trimmed by up to +-1%
 * (<= 17 cents, inaudible) to hold the queue at its target — the
 * standard emulator audio-clock sync, and it absorbs the pacer's
 * -0.28% and any game speed variance for free.
 *
 * Needs dumped DSP firmware (sdmc:/3ds/dspfirm.cdc, e.g. via DSP1).
 * If ndspInit fails the driver declines and out.c falls back to the
 * "none" driver, so a console without firmware just runs silent.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include "out.h"

#define NDSP_CHN      0
#define NBUF          10     /* wave buffers in flight */
#define MAX_FRAMES    2048   /* stereo frames per buffer (46ms cap) */
#define QUEUE_MIN     3      /* ~60ms: survives the 27ms exe spikes */
#define QUEUE_MAX     7      /* ~140ms: heavy scenes (72ms frames) */
#define BASE_RATE     44100.0f
#define RATE_TRIM     0.01f  /* max +-1% */

/* Adaptive cushion: low latency while the emulator keeps up, more
 * buffer when it does not. A frame that takes longer than the queue
 * holds IS a gap no driver can hide, so starvation grows the target
 * and calm stretches shrink it back. */
static int target_queue = QUEUE_MIN;
static int calm_feeds;

static ndspWaveBuf wbuf[NBUF];
static s16 *bufmem;
static int next_buf;
static int inited;
static int need_prime;
static float cur_rate = BASE_RATE;

/* telemetry, drained by the PicaStation frontend heartbeat */
int snd3ds_underruns, snd3ds_overruns, snd3ds_queued;
float snd3ds_rate;
/* kill switch, set by the frontend before SPUinit (sdmc:/psxgpu/sound.off) */
int snd3ds_disable;
int snd3ds_target; /* current adaptive cushion, for telemetry */
int snd3ds_speed_x100 = 100; /* estimated emulation speed, for telemetry */

/* ---- rate-following ("pitch-follow"): the emulator produces samples
 * in EMULATED time, so at 83% speed it makes 83% of real-time audio and
 * no queue can survive that. Play the samples at the rate they arrive:
 * the DSP resamples anyway (44100 -> 32728.5 native), so a different
 * target rate costs zero ARM11 cycles. Music sags in pitch when the
 * game is slow -- the "real hardware under load" sound -- and never
 * gaps. At full speed the estimator reads ~1.0 and behaviour is
 * today's +-trim around 44100. docs/AUDIO_STUTTER_PLAN.md is the
 * derivation; the floor comes from sdmc:/psxgpu/pitchfollow.cfg and
 * pitchfollow.off pins it to 0.99 (the old behaviour) for A/B. */
#define SPD_RING 32              /* ~0.53s at 60 feeds/s: long enough
                                  * that a 10th-frame spike moves the
                                  * mean by its weight, not a warble */
#define TICKS8_PER_S 1047312.0f  /* 268111856 / 256 */
static u32 spd_tick[SPD_RING];   /* svcGetSystemTick() >> 8 */
static u16 spd_fr[SPD_RING];
static int spd_pos, spd_n;
static float spd_est = 1.0f;
static float rate_floor = 0.65f;

/* ---- per-feed event ring: the authoritative continuity record. The
 * .wav capture mirrors what is PRODUCED (continuous by construction);
 * only this ring shows what the DSP was given and when. Written next
 * to the capture as snd_ring.txt. */
struct snd_ev { u32 us; u16 frames; u8 q; u8 flags; float rate; };
#define SND_EV_UNDER   1
#define SND_EV_DROP    2
#define SND_EV_SILENCE 4
#define SND_RING_N 4096          /* ~68s at 60 feeds/s, 48KB heap */
static struct snd_ev *snd_ring;
static unsigned snd_ring_pos;

static void ring_note(int frames, int q, int flags)
{
	struct snd_ev *e;
	if (!snd_ring)
		return;
	e = &snd_ring[snd_ring_pos++ & (SND_RING_N - 1)];
	e->us = (u32)(svcGetSystemTick() / 268); /* ~1us units */
	e->frames = (u16)frames;
	e->q = (u8)q;
	e->flags = (u8)flags;
	e->rate = cur_rate;
}

int snd3ds_ring_dump(const char *path)
{
	FILE *f;
	unsigned i, first, n;
	if (!snd_ring)
		return -1;
	n = snd_ring_pos < SND_RING_N ? snd_ring_pos : SND_RING_N;
	first = snd_ring_pos - n;
	f = fopen(path, "wb");
	if (!f)
		return -1;
	fprintf(f, "us frames q flags rate\n");
	for (i = 0; i < n; i++) {
		struct snd_ev *e = &snd_ring[(first + i) & (SND_RING_N - 1)];
		fprintf(f, "%lu %u %u %u %.1f\n", (unsigned long)e->us,
			e->frames, e->q, e->flags, e->rate);
	}
	fclose(f);
	return (int)n;
}

/* ---- capture: mirror what is fed to the DSP into RAM, written out
 * as a .wav. The only objective way to check the mix remotely (and
 * the user can listen to it). Armed by the frontend, RAM-only. */
#define CAP_MAX_FRAMES (44100 * 6)
static s16 *cap_buf;
static int cap_frames, cap_on;

void snd3ds_capture_start(void)
{
	if (!cap_buf)
		cap_buf = malloc((size_t)CAP_MAX_FRAMES * 2 * sizeof(s16));
	if (!cap_buf)
		return;
	cap_frames = 0;
	cap_on = 1;
}

int snd3ds_capture_done(void)
{
	return cap_buf && !cap_on && cap_frames > 0;
}

/* 16-bit stereo PCM WAV at the nominal rate */
int snd3ds_capture_write(const char *path)
{
	FILE *f;
	u32 data_bytes = (u32)cap_frames * 2 * sizeof(s16), u32v;
	u16 u16v;
	if (!cap_buf || cap_frames <= 0)
		return -1;
	f = fopen(path, "wb");
	if (!f)
		return -1;
	fwrite("RIFF", 1, 4, f);
	u32v = 36 + data_bytes;            fwrite(&u32v, 4, 1, f);
	fwrite("WAVEfmt ", 1, 8, f);
	u32v = 16;                         fwrite(&u32v, 4, 1, f);
	u16v = 1;                          fwrite(&u16v, 2, 1, f); /* PCM */
	u16v = 2;                          fwrite(&u16v, 2, 1, f);
	u32v = 44100;                      fwrite(&u32v, 4, 1, f);
	u32v = 44100 * 4;                  fwrite(&u32v, 4, 1, f);
	u16v = 4;                          fwrite(&u16v, 2, 1, f);
	u16v = 16;                         fwrite(&u16v, 2, 1, f);
	fwrite("data", 1, 4, f);
	fwrite(&data_bytes, 4, 1, f);
	fwrite(cap_buf, 1, data_bytes, f);
	fclose(f);
	/* the continuity record rides along: the wav shows what played,
	 * the ring shows when each feed happened, at what rate, and every
	 * underrun/drop/silence event */
	snd3ds_ring_dump("sdmc:/psxgpu/snd_ring.txt");
	return cap_frames;
}

static int queued_count(void)
{
	int i, n = 0;
	for (i = 0; i < NBUF; i++)
		if (wbuf[i].status == NDSP_WBUF_QUEUED ||
		    wbuf[i].status == NDSP_WBUF_PLAYING)
			n++;
	return n;
}

/* ---- capture v2: OUTPUT-timeline. The old capture mirrored what was
 * produced, which is continuous by construction and therefore proves
 * nothing about stutter. This resamples each block by cur_rate/44100
 * (what the DSP actually does) and appends the silence gaps too, so
 * the wav is what the speaker played: gaps appear as gaps, a slow
 * game appears as pitch. Cost is paid only while a capture is armed. */
static void cap_append(const s16 *src, int frames)
{
	float step = cur_rate / BASE_RATE;
	float pos = 0.0f;
	if (!cap_on || frames < 2)
		return;
	while (cap_frames < CAP_MAX_FRAMES) {
		int i = (int)pos;
		float fr = pos - (float)i;
		const s16 *a;
		s16 *d;
		if (i >= frames - 1)
			break;
		a = src + (size_t)i * 2;
		d = cap_buf + (size_t)cap_frames * 2;
		d[0] = (s16)((float)a[0] + ((float)a[2] - (float)a[0]) * fr);
		d[1] = (s16)((float)a[1] + ((float)a[3] - (float)a[1]) * fr);
		cap_frames++;
		pos += step;
	}
	if (cap_frames >= CAP_MAX_FRAMES)
		cap_on = 0;
}

static void cap_append_silence(int frames)
{
	int n, room;
	if (!cap_on)
		return;
	n = (int)((float)frames * BASE_RATE / cur_rate);
	room = CAP_MAX_FRAMES - cap_frames;
	if (n > room)
		n = room;
	if (n > 0) {
		memset(cap_buf + (size_t)cap_frames * 2, 0,
		       (size_t)n * 2 * sizeof(s16));
		cap_frames += n;
	}
	if (cap_frames >= CAP_MAX_FRAMES)
		cap_on = 0;
}

static int ndsp3ds_init(void)
{
	float mix[12];
	int i;

	if (snd3ds_disable)
		return -1;
	if (R_FAILED(ndspInit()))
		return -1; /* no dspfirm.cdc: caller falls back to silence */

	bufmem = linearAlloc((size_t)NBUF * MAX_FRAMES * 2 * sizeof(s16));
	if (!bufmem) {
		ndspExit();
		return -1;
	}

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnReset(NDSP_CHN);
	ndspChnSetInterp(NDSP_CHN, NDSP_INTERP_LINEAR);
	ndspChnSetRate(NDSP_CHN, BASE_RATE);
	ndspChnSetFormat(NDSP_CHN, NDSP_FORMAT_STEREO_PCM16);
	memset(mix, 0, sizeof(mix));
	mix[0] = mix[1] = 1.0f;
	ndspChnSetMix(NDSP_CHN, mix);

	memset(wbuf, 0, sizeof(wbuf));
	for (i = 0; i < NBUF; i++)
		wbuf[i].status = NDSP_WBUF_DONE; /* all slots free */
	next_buf = 0;
	need_prime = target_queue - 1; /* silence ahead of the first feed */
	cur_rate = BASE_RATE;
	snd3ds_rate = cur_rate;
	{ /* pitch-follow floor, read once at init (boot path, never the
	   * frame path). pitchfollow.cfg = one float 0.50..0.99;
	   * pitchfollow.off pins 0.99 = the old +-trim behaviour, for A/B. */
		FILE *f = fopen("sdmc:/psxgpu/pitchfollow.cfg", "rb");
		if (f) {
			float v = 0.0f;
			if (fscanf(f, "%f", &v) == 1 && v >= 0.50f && v <= 0.99f)
				rate_floor = v;
			fclose(f);
		}
		f = fopen("sdmc:/psxgpu/pitchfollow.off", "rb");
		if (f) {
			fclose(f);
			rate_floor = 0.99f;
		}
	}
	if (!snd_ring)
		snd_ring = malloc(SND_RING_N * sizeof(struct snd_ev));
	snd_ring_pos = 0;
	spd_pos = spd_n = 0;
	spd_est = 1.0f;
	inited = 1;
	return 0;
}

/* queue one buffer of silence to build the initial cushion: without
 * it the queue sits at 1 (a single late frame = an audible gap) */
static void queue_silence(int frames)
{
	ndspWaveBuf *b = &wbuf[next_buf];
	s16 *slot;
	if (b->status != NDSP_WBUF_FREE && b->status != NDSP_WBUF_DONE)
		return;
	slot = bufmem + (size_t)next_buf * MAX_FRAMES * 2;
	memset(slot, 0, (size_t)frames * 2 * sizeof(s16));
	DSP_FlushDataCache(slot, (u32)frames * 2 * sizeof(s16));
	memset(b, 0, sizeof(*b));
	b->data_vaddr = slot;
	b->nsamples = (u32)frames;
	ndspChnWaveBufAdd(NDSP_CHN, b);
	next_buf = (next_buf + 1) % NBUF;
	cap_append_silence(frames);
	ring_note(frames, queued_count(), SND_EV_SILENCE);
}

static void ndsp3ds_finish(void)
{
	if (!inited)
		return;
	inited = 0;
	ndspChnWaveBufClear(NDSP_CHN);
	ndspExit();
	if (bufmem) {
		linearFree(bufmem);
		bufmem = NULL;
	}
}

/* nonzero = enough audio queued; dfsound only consults this with
 * iTempo enabled, where 0 makes it generate extra samples */
static int ndsp3ds_busy(void)
{
	if (!inited)
		return 1;
	return queued_count() >= target_queue;
}

static void ndsp3ds_feed(void *data, int bytes)
{
	ndspWaveBuf *b;
	s16 *slot;
	int frames, q, evflags = 0;

	if (!inited || bytes <= 0)
		return;

	frames = bytes / (2 * (int)sizeof(s16));
	if (frames > MAX_FRAMES)
		frames = MAX_FRAMES;

	{ /* speed estimator: the SPU makes samples in EMULATED time, so
	   * frames-per-wall-second IS the emulation speed. Counted before
	   * the overrun drop below -- a dropped feed was still produced.
	   * A >250ms gap since the last feed (pause, menu, savestate,
	   * stall) clears the window and holds the estimate: the driver
	   * self-heals with no frontend notification needed. */
		u32 now = (u32)(svcGetSystemTick() >> 8);
		if (spd_n > 0) {
			u32 prev = spd_tick[(spd_pos + SPD_RING - 1) % SPD_RING];
			if ((u32)(now - prev) > (u32)(TICKS8_PER_S / 4.0f))
				spd_n = 0;
		}
		spd_tick[spd_pos] = now;
		spd_fr[spd_pos] = (u16)frames;
		spd_pos = (spd_pos + 1) % SPD_RING;
		if (spd_n < SPD_RING)
			spd_n++;
		if (spd_n >= 8) { /* ~130ms of history before trusting it */
			int oldest = (spd_pos + SPD_RING - spd_n) % SPD_RING;
			u32 dt = now - spd_tick[oldest];
			u32 sum = 0;
			int i, idx = oldest;
			for (i = 1; i < spd_n; i++) {
				/* the oldest entry's frames predate its tick */
				idx = (idx + 1) % SPD_RING;
				sum += spd_fr[idx];
			}
			if (dt > 0) {
				float est = (float)sum * TICKS8_PER_S /
					    ((float)dt * BASE_RATE);
				if (est < 0.30f) est = 0.30f;
				if (est > 1.10f) est = 1.10f;
				spd_est = est;
				snd3ds_speed_x100 = (int)(spd_est * 100.0f + 0.5f);
			}
		}
	}

	if (queued_count() == 0) {
		if (!need_prime) {
			snd3ds_underruns++; /* DSP ran dry: real gap */
			evflags |= SND_EV_UNDER;
			if (target_queue < QUEUE_MAX)
				target_queue++; /* buy more cushion */
			calm_feeds = 0;
		}
		need_prime = target_queue - 1;
	} else if (++calm_feeds > 3000) { /* ~60s clean: give latency back */
		calm_feeds = 0;
		if (target_queue > QUEUE_MIN)
			target_queue--;
	}
	while (need_prime > 0) {
		queue_silence(frames);
		need_prime--;
	}

	b = &wbuf[next_buf];
	if (b->status != NDSP_WBUF_FREE && b->status != NDSP_WBUF_DONE) {
		snd3ds_overruns++; /* queue full: drop rather than block */
		ring_note(frames, queued_count(), evflags | SND_EV_DROP);
		return;
	}

	slot = bufmem + (size_t)next_buf * MAX_FRAMES * 2;
	memcpy(slot, data, (size_t)frames * 2 * sizeof(s16));
	DSP_FlushDataCache(slot, (u32)frames * 2 * sizeof(s16));

	cap_append((const s16 *)data, frames); /* output-timeline capture */

	memset(b, 0, sizeof(*b));
	b->data_vaddr = slot;
	b->nsamples = (u32)frames; /* per-channel frames for STEREO_PCM16 */
	ndspChnWaveBufAdd(NDSP_CHN, b);
	next_buf = (next_buf + 1) % NBUF;

	/* Rate-following: the estimator sets the operating point (play the
	 * samples at the rate they actually arrive), the queue error only
	 * trims residual bias. q is sampled after the add, so the
	 * just-queued buffer counts. Falling is quick -- reacting late
	 * costs a dropout -- rising is slower (rising early costs
	 * nothing), and q<=1 may step 2% at once: a fast pitch dip beats
	 * a hole. At full speed spd_est reads ~1.0 and this degenerates
	 * to the old +-trim around 44100. */
	q = queued_count();
	{
		float err = (float)(q - target_queue); /* + = too much latency */
		float want = BASE_RATE * spd_est * (1.0f + err * 0.002f);
		float lo = BASE_RATE * rate_floor;
		float hi = BASE_RATE * 1.02f;
		float d, lim;
		if (want < lo) want = lo;
		if (want > hi) want = hi;
		d = want - cur_rate;
		lim = cur_rate * 0.004f; /* <=0.4%/feed ~ 4 semitones/s: no click */
		if (d > 0.0f) {
			/* near-full queue is the mirror emergency of the empty
			 * one: rising too slowly overflows NBUF and DROPS whole
			 * feeds -- an audible skip, the very thing this exists
			 * to kill (measured over=4/s at spd 68% with the timid
			 * 0.2%/feed rise). */
			float rise = (q >= NBUF - 2) ? cur_rate * 0.02f
						     : lim * 0.5f;
			if (d > rise)
				d = rise;
		} else {
			float fall = (q <= 1) ? cur_rate * 0.02f : lim;
			if (-d > fall)
				d = -fall;
		}
		cur_rate += d;
		ndspChnSetRate(NDSP_CHN, cur_rate);
		snd3ds_rate = cur_rate;
		snd3ds_queued = q;
		snd3ds_target = target_queue;
	}
	ring_note(frames, q, evflags);
}

void out_register_ndsp3ds(struct out_driver *drv)
{
	drv->name = "ndsp3ds";
	drv->init = ndsp3ds_init;
	drv->finish = ndsp3ds_finish;
	drv->busy = ndsp3ds_busy;
	drv->feed = ndsp3ds_feed;
}
