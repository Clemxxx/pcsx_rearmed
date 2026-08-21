/***************************************************************************
 *   PCSX-Revolution - PlayStation Emulator for Nintendo Wii               *
 *   Copyright (C) 2009-2010  PCSX-Revolution Dev Team                     *
 *   <http://code.google.com/p/pcsx-revolution/>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#ifndef __GTE_H__
#define __GTE_H__

enum gteop_opcodes {
	GTEOP_RTPS  = 0x01,
	GTEOP_NCLIP = 0x06,
	GTEOP_OP    = 0x0c,
	GTEOP_DPCS  = 0x10,
	GTEOP_INTPL = 0x11,
	GTEOP_MVMVA = 0x12,
	GTEOP_NCDS  = 0x13,
	GTEOP_CDP   = 0x14,
	GTEOP_NCDT  = 0x16,
	GTEOP_NCCS  = 0x1b,
	GTEOP_CC    = 0x1c,
	GTEOP_NCS   = 0x1e,
	GTEOP_NCT   = 0x20,
	GTEOP_SQR   = 0x28,
	GTEOP_DCPL  = 0x29,
	GTEOP_DPCT  = 0x2a,
	GTEOP_AVSZ3 = 0x2d,
	GTEOP_AVSZ4 = 0x2e,
	GTEOP_RTPT  = 0x30,
	GTEOP_GPF   = 0x3d,
	GTEOP_GPL   = 0x3e,
	GTEOP_NCCT  = 0x3f,
};

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "r3000a.h"

struct psxCP2Regs;

extern const unsigned char gte_cycletab[64];

void gteCheckStall(u32 op);
void gteDispatch(psxCP2Regs *regs, u32 code);

u32  MFC2(struct psxCP2Regs *regs, int reg);
void MTC2(struct psxCP2Regs *regs, u32 value, int reg);
void CTC2(struct psxCP2Regs *regs, u32 value, int reg);

typedef void (gte_handler)(psxCP2Regs *regs, u32 code);

/* PGXP-style geometry capture, implemented in gte.c. When enabled,
 * RTPS/RTPT keep the sub-pixel projection they would otherwise discard.
 * The recompiler reads this to decide whether it may substitute the asm
 * fast paths for those two ops (see assem_arm.c). */
extern int pgxp_capture_on;
int pgxp_lookup(u32 packed, float *x, float *y, float *z);
/* the pre-projection view-space vector, if we have it */
int pgxp_lookup_v(u32 packed, float *vx, float *vy, float *vz);
/* the projection parameters that vertex was transformed with */
int pgxp_lookup_proj(u32 packed, float *ofx, float *ofy, float *h);
void pgxp_frame(void);   /* call once per emulated frame */
gte_handler *gteGetHandler(u32 code);
gte_handler *gteGetHandler_nf(u32 code);

// used by asm/drc
void gteRTPS_sf1lm0(psxCP2Regs *regs, u32 code);
void gteMVMVA_generic(psxCP2Regs *regs, u32 code);
void gteMVMVA_generic_nf(psxCP2Regs *regs, u32 code);

void gteSQR_part_noshift(struct psxCP2Regs *regs);
void gteSQR_part_shift(struct psxCP2Regs *regs);
void gteOP_part_noshift(struct psxCP2Regs *regs);
void gteOP_part_shift(struct psxCP2Regs *regs);
void gteGPF_part_noshift(struct psxCP2Regs *regs);
void gteGPF_part_shift(struct psxCP2Regs *regs);
void gteGPL_part_noshift(struct psxCP2Regs *regs);

void gteGPL_part_shift(struct psxCP2Regs *regs);
void gteGPL_part_shift_nf(struct psxCP2Regs *regs);
void gteDPCS_part_noshift(struct psxCP2Regs *regs);
void gteDPCS_part_noshift_nf(struct psxCP2Regs *regs);
void gteDPCS_part_shift(struct psxCP2Regs *regs);
void gteDPCS_part_shift_nf(struct psxCP2Regs *regs);
void gteINTPL_part_noshift(struct psxCP2Regs *regs);
void gteINTPL_part_noshift_nf(struct psxCP2Regs *regs);
void gteINTPL_part_shift(struct psxCP2Regs *regs);
void gteINTPL_part_shift_nf(struct psxCP2Regs *regs);
void gteMACtoRGB(struct psxCP2Regs *regs);
void gteMACtoRGB_nf(struct psxCP2Regs *regs);

/* Address provenance: pgxp_store() records where a GTE result was
 * written; pgxp_addr_lookup() matches a display-list word back to the
 * exact transform that produced it. Unlike the screen coordinate, an
 * address is unique, so this has no ambiguity to resolve.
 *
 * pgxp_shadow_alloc() must be called before either is useful; until it
 * is, pgxp_store() is a no-op and every lookup misses. It is also what
 * the recompiler tests to decide whether to emit the SWC2 hook, so it
 * has to run during GPUinit, before any game code is compiled. */
extern int pgxp_addr_on;
/* memory mode: propagate the tag through mfc2 / lw / sw so it
 * survives the copy from the GTE work buffer into the display list.
 * Read by the recompiler at block-compile time, same as above. */
extern int pgxp_mem_on;
int  pgxp_mem_enable(void);
/* returns 1 if the state changed and the block cache must be dropped */
int  pgxp_mem_set(int on);
void pgxp_mfc2(u32 rt, u32 creg);
void pgxp_mem_load(u32 addr, u32 rt);
void pgxp_mem_store(u32 addr, u32 rt, u32 val);
int  pgxp_shadow_alloc(void);
void pgxp_store(u32 addr, int creg, u32 val);
int pgxp_addr_lookup(u32 addr, u32 val, float *x, float *y, float *z,
                     float *vx, float *vy, float *vz,
                     float *ofx, float *ofy, float *h);

#ifdef __cplusplus
}
#endif
#endif
