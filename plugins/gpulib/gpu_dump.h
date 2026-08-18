/*
 * GPU command-stream dump recorder (PicaStation harness).
 * Enabled at runtime when the PSXGPU_DUMP env var holds an output path.
 * Format: docs/DUMP_FORMAT.md in the PicaStation repo (magic "PSXGDUMP" v1).
 */
#ifndef __GPULIB_GPU_DUMP_H__
#define __GPULIB_GPU_DUMP_H__

#include <stdint.h>

struct psx_gpu;
struct rearmed_cbs;

#ifdef GPULIB_DUMP_RECORDER

void gpu_dump_config(const struct rearmed_cbs *cbs);
void gpu_dump_gp0(struct psx_gpu *gpu, const uint32_t *data, int count);
void gpu_dump_gp1(struct psx_gpu *gpu, uint32_t data);
void gpu_dump_read(struct psx_gpu *gpu, int count);
void gpu_dump_vblank(struct psx_gpu *gpu);
void gpu_dump_finish(void);

#else

#define gpu_dump_config(cbs) do {} while (0)
#define gpu_dump_gp0(gpu, data, count) do {} while (0)
#define gpu_dump_gp1(gpu, data) do {} while (0)
#define gpu_dump_read(gpu, count) do {} while (0)
#define gpu_dump_vblank(gpu) do {} while (0)
#define gpu_dump_finish() do {} while (0)

#endif

#endif /* __GPULIB_GPU_DUMP_H__ */
