/*
 * GPU command-stream dump recorder (PicaStation harness).
 * See gpu_dump.h. Not built for console targets; desktop harness only.
 */
#ifdef GPULIB_DUMP_RECORDER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpu.h"
#include "gpu_dump.h"

#define DUMP_MAGIC "PSXGDUMP"
#define DUMP_VERSION 1
#define VRAM_BYTES (1024 * 512 * 2)
#define SNAPSHOT_INTERVAL 120

enum {
  CHUNK_VRAM_SNAPSHOT = 0x01,
  CHUNK_GP0           = 0x02,
  CHUNK_GP1           = 0x03,
  CHUNK_FRAME         = 0x04,
  CHUNK_VRAM_HASH     = 0x05,
  CHUNK_META          = 0x06,
  CHUNK_GP0_READ      = 0x07,
};

static FILE *dump_file;
static int dump_checked;
static uint32_t dump_frame;

static uint64_t fnv1a64(const void *buf, size_t len)
{
  const unsigned char *p = buf;
  uint64_t h = 0xcbf29ce484222325ull;
  size_t i;
  for (i = 0; i < len; i++) {
    h ^= p[i];
    h *= 0x100000001b3ull;
  }
  return h;
}

static void chunk(uint32_t type, const void *payload, uint32_t size)
{
  fwrite(&type, 4, 1, dump_file);
  fwrite(&size, 4, 1, dump_file);
  if (size)
    fwrite(payload, 1, size, dump_file);
}

static void snapshot(struct psx_gpu *gpu)
{
  if (gpu->vram)
    chunk(CHUNK_VRAM_SNAPSHOT, gpu->vram, VRAM_BYTES);
}

static int dump_active(struct psx_gpu *gpu)
{
  const char *path;
  uint32_t v, z = 0;

  if (dump_file)
    return 1;
  if (dump_checked)
    return 0;
  dump_checked = 1;
  path = getenv("PSXGPU_DUMP");
  if (!path || !path[0])
    return 0;
  dump_file = fopen(path, "wb");
  if (!dump_file) {
    fprintf(stderr, "gpu_dump: can't open %s\n", path);
    return 0;
  }
  fwrite(DUMP_MAGIC, 1, 8, dump_file);
  v = DUMP_VERSION;
  fwrite(&v, 4, 1, dump_file);
  fwrite(&z, 4, 1, dump_file);
  fwrite(&z, 4, 1, dump_file);
  fwrite(&z, 4, 1, dump_file);
  {
    const char meta[] = "recorder: pcsx_rearmed gpulib (picastation branch)";
    chunk(CHUNK_META, meta, sizeof(meta) - 1);
  }
  snapshot(gpu);
  fprintf(stderr, "gpu_dump: recording to %s\n", path);
  return 1;
}

void gpu_dump_gp0(struct psx_gpu *gpu, const uint32_t *data, int count)
{
  if (count <= 0 || !dump_active(gpu))
    return;
  chunk(CHUNK_GP0, data, (uint32_t)count * 4);
}

void gpu_dump_gp1(struct psx_gpu *gpu, uint32_t data)
{
  if (!dump_active(gpu))
    return;
  chunk(CHUNK_GP1, &data, 4);
}

void gpu_dump_read(struct psx_gpu *gpu, int count)
{
  uint32_t c = count;
  if (count <= 0 || !dump_active(gpu))
    return;
  chunk(CHUNK_GP0_READ, &c, 4);
}

void gpu_dump_vblank(struct psx_gpu *gpu)
{
  if (!dump_active(gpu))
    return;
  {
    uint32_t frame[6];
    frame[0] = dump_frame;
    frame[1] = gpu->status;
    frame[2] = (uint16_t)gpu->screen.src_x | ((uint32_t)(uint16_t)gpu->screen.src_y << 16);
    frame[3] = (uint16_t)gpu->screen.x1 | ((uint32_t)(uint16_t)gpu->screen.x2 << 16);
    frame[4] = (uint16_t)gpu->screen.y1 | ((uint32_t)(uint16_t)gpu->screen.y2 << 16);
    frame[5] = (uint16_t)gpu->screen.hres | ((uint32_t)(uint16_t)gpu->screen.vres << 16);
    chunk(CHUNK_FRAME, frame, sizeof(frame));
  }
  if (gpu->vram) {
    struct { uint32_t frame; uint32_t pad; uint64_t hash; } h;
    h.frame = dump_frame;
    h.pad = 0;
    h.hash = fnv1a64(gpu->vram, VRAM_BYTES);
    chunk(CHUNK_VRAM_HASH, &h, sizeof(h));
  }
  dump_frame++;
  if (dump_frame % SNAPSHOT_INTERVAL == 0)
    snapshot(gpu);
}

void gpu_dump_finish(void)
{
  if (dump_file) {
    fclose(dump_file);
    dump_file = NULL;
    fprintf(stderr, "gpu_dump: closed after %u frames\n", (unsigned)dump_frame);
  }
}

#endif /* GPULIB_DUMP_RECORDER */
