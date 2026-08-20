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
	int frames, q;

	if (!inited || bytes <= 0)
		return;

	frames = bytes / (2 * (int)sizeof(s16));
	if (frames > MAX_FRAMES)
		frames = MAX_FRAMES;

	if (queued_count() == 0) {
		if (!need_prime) {
			snd3ds_underruns++; /* DSP ran dry: real gap */
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
		return;
	}

	slot = bufmem + (size_t)next_buf * MAX_FRAMES * 2;
	memcpy(slot, data, (size_t)frames * 2 * sizeof(s16));
	DSP_FlushDataCache(slot, (u32)frames * 2 * sizeof(s16));

	if (cap_on) { /* mirror the exact stream for offline inspection */
		int room = CAP_MAX_FRAMES - cap_frames;
		int n = frames < room ? frames : room;
		if (n > 0) {
			memcpy(cap_buf + (size_t)cap_frames * 2, data,
			       (size_t)n * 2 * sizeof(s16));
			cap_frames += n;
		}
		if (cap_frames >= CAP_MAX_FRAMES)
			cap_on = 0;
	}

	memset(b, 0, sizeof(*b));
	b->data_vaddr = slot;
	b->nsamples = (u32)frames; /* per-channel frames for STEREO_PCM16 */
	ndspChnWaveBufAdd(NDSP_CHN, b);
	next_buf = (next_buf + 1) % NBUF;

	/* audio-clock sync: nudge playback rate toward the target depth.
	 * q is sampled after the add, so the just-queued buffer counts. */
	q = queued_count();
	{
		float err = (float)(q - target_queue); /* + = too much latency */
		float want = BASE_RATE * (1.0f + err * 0.002f);
		float lo = BASE_RATE * (1.0f - RATE_TRIM);
		float hi = BASE_RATE * (1.0f + RATE_TRIM);
		if (want < lo) want = lo;
		if (want > hi) want = hi;
		/* slew so the pitch never steps audibly */
		cur_rate += (want - cur_rate) * 0.05f;
		ndspChnSetRate(NDSP_CHN, cur_rate);
		snd3ds_rate = cur_rate;
		snd3ds_queued = q;
		snd3ds_target = target_queue;
	}
}

void out_register_ndsp3ds(struct out_driver *drv)
{
	drv->name = "ndsp3ds";
	drv->init = ndsp3ds_init;
	drv->finish = ndsp3ds_finish;
	drv->busy = ndsp3ds_busy;
	drv->feed = ndsp3ds_feed;
}
