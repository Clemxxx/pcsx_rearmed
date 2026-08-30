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

/*
* GTE functions.
*/

#include <stdlib.h>
#include "gte.h"
#include "gte_arm.h"
#include "psxmem.h"
#include "../include/compiler_features.h"
#include "../include/arm_features.h"

#ifndef GTE_LOG
#define GTE_LOG(...)
#endif

#ifdef FLAGLESS
#define NF _nf
#define NM(n) n##_nf
#else
#define NF
#define NM(n) n
#endif

#define VX(n) (n < 3 ? regs->CP2D.p[n << 1].sw.l : regs->CP2D.p[9].sw.l)
#define VY(n) (n < 3 ? regs->CP2D.p[n << 1].sw.h : regs->CP2D.p[10].sw.l)
#define VZ(n) (n < 3 ? regs->CP2D.p[(n << 1) + 1].sw.l : regs->CP2D.p[11].sw.l)

#define fSX(n) ((regs->CP2D.p)[((n) + 12)].sw.l)
#define fSY(n) ((regs->CP2D.p)[((n) + 12)].sw.h)
#define fSZ(n) ((regs->CP2D.p)[((n) + 17)].w.l) /* (n == 0) => SZ1; */

#define fR(x)    (regs->CP2D.p[(x) + 20].b.l)
#define fG(x)    (regs->CP2D.p[(x) + 20].b.h)
#define fB(x)    (regs->CP2D.p[(x) + 20].b.h2)
#define fCODE(x) (regs->CP2D.p[(x) + 20].b.h3)

#define gteVXY0 (regs->CP2D.r[0])
#define gteVX0  (regs->CP2D.p[0].sw.l)
#define gteVY0  (regs->CP2D.p[0].sw.h)
#define gteVZ0  (regs->CP2D.p[1].sw.l)
#define gteVXY1 (regs->CP2D.r[2])
#define gteVX1  (regs->CP2D.p[2].sw.l)
#define gteVY1  (regs->CP2D.p[2].sw.h)
#define gteVZ1  (regs->CP2D.p[3].sw.l)
#define gteVXY2 (regs->CP2D.r[4])
#define gteVX2  (regs->CP2D.p[4].sw.l)
#define gteVY2  (regs->CP2D.p[4].sw.h)
#define gteVZ2  (regs->CP2D.p[5].sw.l)
#define gteRGB  (regs->CP2D.r[6])
#define gteR    (regs->CP2D.p[6].b.l)
#define gteG    (regs->CP2D.p[6].b.h)
#define gteB    (regs->CP2D.p[6].b.h2)
#define gteCODE (regs->CP2D.p[6].b.h3)
#define gteOTZ  (regs->CP2D.p[7].w.l)
#define gteIR0  (regs->CP2D.p[8].sw.l)
#define gteIR1  (regs->CP2D.p[9].sw.l)
#define gteIR2  (regs->CP2D.p[10].sw.l)
#define gteIR3  (regs->CP2D.p[11].sw.l)
#define gteSXY0 (regs->CP2D.r[12])
#define gteSX0  (regs->CP2D.p[12].sw.l)
#define gteSY0  (regs->CP2D.p[12].sw.h)
#define gteSXY1 (regs->CP2D.r[13])
#define gteSX1  (regs->CP2D.p[13].sw.l)
#define gteSY1  (regs->CP2D.p[13].sw.h)
#define gteSXY2 (regs->CP2D.r[14])
#define gteSX2  (regs->CP2D.p[14].sw.l)
#define gteSY2  (regs->CP2D.p[14].sw.h)
#define gteSXYP (regs->CP2D.r[15])
#define gteSXP  (regs->CP2D.p[15].sw.l)
#define gteSYP  (regs->CP2D.p[15].sw.h)
#define gteSZ0  (regs->CP2D.p[16].w.l)
#define gteSZ1  (regs->CP2D.p[17].w.l)
#define gteSZ2  (regs->CP2D.p[18].w.l)
#define gteSZ3  (regs->CP2D.p[19].w.l)
#define gteRGB0  (regs->CP2D.r[20])
#define gteR0    (regs->CP2D.p[20].b.l)
#define gteG0    (regs->CP2D.p[20].b.h)
#define gteB0    (regs->CP2D.p[20].b.h2)
#define gteCODE0 (regs->CP2D.p[20].b.h3)
#define gteRGB1  (regs->CP2D.r[21])
#define gteR1    (regs->CP2D.p[21].b.l)
#define gteG1    (regs->CP2D.p[21].b.h)
#define gteB1    (regs->CP2D.p[21].b.h2)
#define gteCODE1 (regs->CP2D.p[21].b.h3)
#define gteRGB2  (regs->CP2D.r[22])
#define gteR2    (regs->CP2D.p[22].b.l)
#define gteG2    (regs->CP2D.p[22].b.h)
#define gteB2    (regs->CP2D.p[22].b.h2)
#define gteCODE2 (regs->CP2D.p[22].b.h3)
#define gteRES1  (regs->CP2D.r[23])
#define gteMAC0  (((s32 *)regs->CP2D.r)[24])
#define gteMAC1  (((s32 *)regs->CP2D.r)[25])
#define gteMAC2  (((s32 *)regs->CP2D.r)[26])
#define gteMAC3  (((s32 *)regs->CP2D.r)[27])
#define gteIRGB  (regs->CP2D.r[28])
#define gteORGB  (regs->CP2D.r[29])
#define gteLZCS  (regs->CP2D.r[30])
#define gteLZCR  (regs->CP2D.r[31])

#define gteR11R12 (((s32 *)regs->CP2C.r)[0])
#define gteR22R23 (((s32 *)regs->CP2C.r)[2])
#define gteR11 (regs->CP2C.p[0].sw.l)
#define gteR12 (regs->CP2C.p[0].sw.h)
#define gteR13 (regs->CP2C.p[1].sw.l)
#define gteR21 (regs->CP2C.p[1].sw.h)
#define gteR22 (regs->CP2C.p[2].sw.l)
#define gteR23 (regs->CP2C.p[2].sw.h)
#define gteR31 (regs->CP2C.p[3].sw.l)
#define gteR32 (regs->CP2C.p[3].sw.h)
#define gteR33 (regs->CP2C.p[4].sw.l)
#define gteTRX (((s32 *)regs->CP2C.r)[5])
#define gteTRY (((s32 *)regs->CP2C.r)[6])
#define gteTRZ (((s32 *)regs->CP2C.r)[7])
#define gteL11 (regs->CP2C.p[8].sw.l)
#define gteL12 (regs->CP2C.p[8].sw.h)
#define gteL13 (regs->CP2C.p[9].sw.l)
#define gteL21 (regs->CP2C.p[9].sw.h)
#define gteL22 (regs->CP2C.p[10].sw.l)
#define gteL23 (regs->CP2C.p[10].sw.h)
#define gteL31 (regs->CP2C.p[11].sw.l)
#define gteL32 (regs->CP2C.p[11].sw.h)
#define gteL33 (regs->CP2C.p[12].sw.l)
#define gteRBK (((s32 *)regs->CP2C.r)[13])
#define gteGBK (((s32 *)regs->CP2C.r)[14])
#define gteBBK (((s32 *)regs->CP2C.r)[15])
#define gteLR1 (regs->CP2C.p[16].sw.l)
#define gteLR2 (regs->CP2C.p[16].sw.h)
#define gteLR3 (regs->CP2C.p[17].sw.l)
#define gteLG1 (regs->CP2C.p[17].sw.h)
#define gteLG2 (regs->CP2C.p[18].sw.l)
#define gteLG3 (regs->CP2C.p[18].sw.h)
#define gteLB1 (regs->CP2C.p[19].sw.l)
#define gteLB2 (regs->CP2C.p[19].sw.h)
#define gteLB3 (regs->CP2C.p[20].sw.l)
#define gteRFC (((s32 *)regs->CP2C.r)[21])
#define gteGFC (((s32 *)regs->CP2C.r)[22])
#define gteBFC (((s32 *)regs->CP2C.r)[23])
#define gteOFX (((s32 *)regs->CP2C.r)[24])
#define gteOFY (((s32 *)regs->CP2C.r)[25])
// senquack - gteH register is u16, not s16, and used in GTE that way.
//  HOWEVER when read back by CPU using CFC2, it will be incorrectly
//  sign-extended by bug in original hardware, according to Nocash docs
//  GTE section 'Screen Offset and Distance'. The emulator does this
//  sign extension when it is loaded to GTE by CTC2.
//#define gteH   (regs->CP2C.p[26].sw.l)
#define gteH   (regs->CP2C.p[26].w.l)
#define gteDQA (regs->CP2C.p[27].sw.l)
#define gteDQB (((s32 *)regs->CP2C.r)[28])
#define gteZSF3 (regs->CP2C.p[29].sw.l)
#define gteZSF4 (regs->CP2C.p[30].sw.l)
#define gteFLAG (regs->CP2C.r[31])

#define GTE_SF(op) ((op >> 19) & 1)
#define GTE_MX(op) ((op >> 17) & 3)
#define GTE_V(op) ((op >> 15) & 3)
#define GTE_CV(op) ((op >> 13) & 3)
#define GTE_CD(op) ((op >> 11) & 3) /* not used */
#define GTE_LM(op) ((op >> 10) & 1)
#define GTE_CT(op) ((op >> 6) & 15) /* not used */

// shift the gte 44bit accumulator to 64bit
#define MAC123_SHIFT (32-12)

// mac 123 without flags (expensive to calculate, rarely used)
// if your platform is slow, consider adding it here
#if defined(FLAGLESS) || (defined(__arm__) && !defined(HAVE_ARMV5))

static inline s64 mac123add4(u32 id, u32 *flags, s32 a1, s32 a2, s32 a3, s32 a4, int shift) {
	return (((s64)a1 << 12) + a2 + a3 + a4) >> shift;
}

static inline s32 mac123add_s12(u32 id, u32 *flags, s32 in12, s32 addend, int shift) {
	return (((s64)in12 << 12) + addend) >> shift;
}

static inline s32 mac123sub_s12(u32 id, u32 *flags, s32 in12, s32 subtrahend, int shift) {
	return (((s64)in12 << 12) - subtrahend) >> shift;
}

#else

static inline s64 mac123add(u32 id, u32 *flags, s64 in, s32 addend) {
	s64 a;
#if defined(__arm__)
	u32 flag = 1u << (31 - id);
	asm("adds %Q[a], %Q[in], %[add], lsl #20\n"
	    "adcs %R[a], %R[in], %[add], asr #12\n"
	    "movpl %[flag], %[flag], lsr #3\n"
	    "orrvs %[flags], %[flags], %[flag]"
	    : [a]"=&r"(a), [flags]"+&r"(*flags), [flag]"+&r"(flag)
	    : [in]"r"(in), [add]"r"(addend)
	    : "cc");
#elif 0 // defined(__aarch64__) // slower
	u32 flag = 1u << (31 - id);
	u32 flagpl = 1u << (31 - id - 3);
	s64 add_ = addend;
	asm("adds %[a], %[in], %[add], lsl %[shift]\n"
	    "csel %w[flag], %w[flagpl], %w[flag], pl\n"
	    "csel %w[flag], %w[flag], wzr, vs\n"
	    : [a]"=&r"(a), [flag]"+&r"(flag)
	    : [in]"r"(in), [add]"r"(add_), [flagpl]"r"(flagpl), [shift]"i"(MAC123_SHIFT)
	    : "cc");
	*flags |= flag;
#else
	int o = __builtin_add_overflow(in, (s64)addend << MAC123_SHIFT, &a);
	*flags |= (o && a <  0) << (31 - id);
	*flags |= (o && a >= 0) << (28 - id);
#endif
	return a;
}

static inline s64 mac123add4(u32 id, u32 *flags, s32 a1, s32 a2, s32 a3, s32 a4, int shift) {
	s64 a = (s64)a1 << (12 + MAC123_SHIFT);
	a = mac123add(id, flags, a, a2);
	a = mac123add(id, flags, a, a3);
	a = mac123add(id, flags, a, a4);
	return a >> (shift + MAC123_SHIFT);
}

static inline s32 mac123add_s12(u32 id, u32 *flags, s32 in12, s32 addend, int shift) {
	return mac123add(id, flags, (s64)in12 << (12+MAC123_SHIFT), addend) >> (shift+MAC123_SHIFT);
}

static inline s64 mac123sub_s12(u32 id, u32 *flags, s32 in12, s32 subtrahend, int shift) {
	s64 a;
#if defined(__arm__)
	u32 flag = 1u << (31 - id);
	s64 in = (s64)in12 << (12+MAC123_SHIFT);
	asm("subs %Q[a], %Q[in], %[sub], lsl #20\n"
	    "sbcs %R[a], %R[in], %[sub], asr #12\n"
	    "movpl %[flag], %[flag], lsr #3\n"
	    "orrvs %[flags], %[flags], %[flag]"
	    : [a]"=&r"(a), [flags]"+&r"(*flags), [flag]"+&r"(flag)
	    : [in]"r"(in), [sub]"r"(subtrahend)
	    : "cc");
#elif 0 // defined(__aarch64__)
	u32 flag = 1u << (31 - id);
	u32 flagpl = 1u << (31 - id - 3);
	s64 in = (s64)in12 << (12+MAC123_SHIFT);
	s64 sub_ = subtrahend;
	asm("subs %[a], %[in], %[sub], lsl %[shift]\n"
	    "csel %w[flag], %w[flagpl], %w[flag], pl\n"
	    "csel %w[flag], %w[flag], wzr, vs\n"
	    : [a]"=&r"(a), [flag]"+&r"(flag)
	    : [in]"r"(in), [sub]"r"(sub_), [flagpl]"r"(flagpl), [shift]"i"(MAC123_SHIFT)
	    : "cc");
	*flags |= flag;
#else
	int o = __builtin_sub_overflow((s64)in12 << (12+MAC123_SHIFT), (s64)subtrahend << MAC123_SHIFT, &a);
	*flags |= (o && subtrahend <  0) << (31 - id);
	*flags |= (o && subtrahend >= 0) << (28 - id);
#endif
	return a >> (shift+MAC123_SHIFT);
}

#endif // !FLAGLESS for mac 123

/* ---- DEPTH TAG ---------------------------------------------------
 * The GPU reads only 11 bits of each 16-bit screen coordinate:
 *     ps1gpu.c   x = sext11(w & 0x7FF)
 * so bits 11..15 of each half -- ten bits per vertex -- are discarded
 * by the hardware. The GTE clamps its output to [-1024,1023] (limG1),
 * which is exactly 11 bits signed, so those ten bits carry nothing but
 * sign extension.
 *
 * Put the vertex's own depth in them. The game then carries it to the
 * display list for us, inside a word it treats as opaque -- through
 * lw/sw, memcpy, the ordering table and the DMA -- and the GPU side
 * reads the depth straight out of the packet.
 *
 * That removes the matching problem completely: there is no table to
 * look up, nothing to key on, and no vertex that can be missed,
 * because every vertex carries its own answer. What it costs is the
 * five spare bits per coordinate, which is only safe while the game
 * treats the word as opaque -- see the checksum below.
 *
 * Written into the SXY FIFO itself, not into MFC2's return value, so
 * that the recompiler (which reads the register file directly) sees it
 * without any emitted call. The one GTE op that reads the FIFO back is
 * NCLIP, which masks to 11 bits when this is on.
 */
/* sign-extend a tagged coordinate from bit 10 */
static s32 tag11(s32 v) { return (s32)((u32)v << 21) >> 21; }

/* 8-bit floating depth: 4-bit exponent, 4-bit mantissa. 16 steps per
 * octave is 4.4% relative error, which at a 20px parallax budget is
 * under a pixel -- and the panel cannot show sub-pixel parallax
 * anyway (docs/STEREO_3D_NOTES.md). */
static u32 gte_depth_code(u32 sz)
{
	int e;
	if (sz > 0xffff)
		sz = 0xffff;
	if (sz < 16)
		return sz;                 /* codes 0..15 are exact */
	for (e = 15; e > 4; e--)
		if (sz >> e)
			break;
	return (u32)((e << 4) | ((sz >> (e - 4)) & 0xf));
}

/* Two bits of checksum over the coordinate AND the depth. Its job is
 * not to catch random data -- untagged coordinates are sign-extended,
 * so their tag bits are all-0 or all-1 and stand out -- but to catch a
 * word the GAME modified. If a game adds an offset to the packed
 * coordinate, the checksum no longer matches and the vertex falls back
 * to flat, which is the same fail-safe rule PGXP uses. */
static u32 gte_tag_sum(u32 x, u32 y, u32 d)
{
	u32 s = x * 5u + y * 3u + d * 7u;
	s ^= s >> 5;
	s ^= s >> 3;
	return (s + 1u) & 3u;        /* +1: never 0 for an all-zero vertex */
}

/* BISECT: everything the tag switches on -- the C handlers for
 * RTPS/RTPT/NCLIP, the NCLIP masking, the decode path -- but the word
 * itself left exactly as the GTE produced it. If the picture is still
 * wrong with this on, the fault is in those changes and not in the
 * bits we borrowed. */
static u32 gte_tag_encode(u32 xy, u32 sz)
{
	u32 x = xy & 0x7ffu, y = (xy >> 16) & 0x7ffu;
	if (gte_tag_null)
		return xy;
	u32 d = gte_depth_code(sz);
	u32 t = (gte_tag_sum(x, y, d) << 8) | d;
	return (x | ((t & 0x1fu) << 11)) |
	       ((y | (((t >> 5) & 0x1fu) << 11)) << 16);
}

#ifndef FLAGLESS

static inline s64 mac0flags(u32 *flags, s64 a) {
#if 1
	if (a != (s32)a)
		*flags |= 1u << (16 + (a >> 63));
#else
	if (a > 0x7fffffff)
		*flags |= 1u << 16;
	if (a < -(s64)0x80000000)
		*flags |= 1u << 15;
#endif
	return a;
}

#if defined(__arm__)

#define LIM(flags_, value_, max_, min_, flag_) \
({s32 r_ = value_; \
  asm("cmp   %[val], %[max]\n" \
      "movgt %[val], %[max]\n" \
      "orrgt %[flags], %[flag]\n" \
      "cmp   %[val], %[min]\n" \
      "movlt %[val], %[min]\n" \
      "orrlt %[flags], %[flag]\n" \
      : [val]"+&r"(r_), [flags]"+&r"(*(flags_)) \
      : [max]"r"(max_), [min]"r"(min_), [flag]"i"(flag_) \
      : "cc"); \
  r_;})

#elif defined(__aarch64__)

#define LIM(flags_, value_, max_, min_, flagc_) \
({s32 r_ = value_; \
  u32 flag_o_, flag_ = flagc_; \
  asm("cmp  %w[val], %w[max]\n" \
      "csel %w[val], %w[max], %w[val], gt\n" \
      "csel %w[flag_o], %w[flag], wzr, gt\n" \
      "cmp  %w[val], %w[min]\n" \
      "csel %w[val], %w[min], %w[val], lt\n" \
      "csel %w[flag_o], %w[flag], %w[flag_o], lt\n" \
      : [val]"+&r"(r_), [flag_o]"=&r"(flag_o_) \
      : [max]"r"(max_), [min]"r"(min_), [flag]"r"(flag_) \
      : "cc"); \
  *(flags_) |= flag_o_; \
  r_;})

#else

static inline s32 LIM(u32 *flags, s32 value, s32 max, s32 min, u32 flag) {
	s32 ret = value;
	if (ret > max)
		ret = max;
	if (ret < min)
		ret = min;
	if (ret != value)
		*flags |= flag;
	return ret;
}

#endif

static inline void LIMF(u32 *flags, s32 value, s32 max, s32 min, u32 flag) {
	if (value > max || value < min)
		*flags |= flag;
}

static inline u32 getFinalFlag(u32 flags) {
	flags |= ~((flags & 0x7f87e000u) - 1) & (1u << 31);
	return flags;
}

#else

static inline s64 mac0flags(u32 *flags, s64 a) {
	return a;
}

static inline s32 LIM(u32 *flags, s32 value, s32 max, s32 min, u32 flag) {
	s32 ret = value;
	if (ret > max)
		ret = max;
	if (ret < min)
		ret = min;
	return ret;
}

#define LIMF(flags, a, ...) (void)(a)

static inline u32 getFinalFlag(u32 flags) {
	return 0;
}

#endif

#define limB1(flags, a, l)   LIM(flags, a, 0x7fff, -0x8000 * !l, (1u << 24))
#define limB2(flags, a, l)   LIM(flags, a, 0x7fff, -0x8000 * !l, (1u << 23))
#define limB3(flags, a, l)   LIM(flags, a, 0x7fff, -0x8000 * !l, (1u << 22))
#define limBF1(flags, a, l) LIMF(flags, a, 0x7fff, -0x8000 * !l, (1u << 24))
#define limBF2(flags, a, l) LIMF(flags, a, 0x7fff, -0x8000 * !l, (1u << 23))
#define limBF3(flags, a, l) LIMF(flags, a, 0x7fff, -0x8000 * !l, (1u << 22))
#define limC1(flags, a) LIM(flags, a, 0x00ff, 0x0000, (1u << 21))
#define limC2(flags, a) LIM(flags, a, 0x00ff, 0x0000, (1u << 20))
#define limC3(flags, a) LIM(flags, a, 0x00ff, 0x0000, (1u << 19))
#define limD(flags, a)  LIM(flags, a, 0xffff, 0x0000, (1u << 18))
#define limG1(flags, a) LIM(flags, a,  0x3ff, -0x400, (1u << 14))
#define limG2(flags, a) LIM(flags, a,  0x3ff, -0x400, (1u << 13))
#define limH(flags, a)  LIM(flags, a, 0x1000, 0x0000, (1u << 12))

//senquack - n param should be unsigned (will be 'gteH' reg which is u16)
#ifdef GTE_USE_NATIVE_DIVIDE
INLINE u32 DIVIDE(u16 n, u16 d) {
	return ((u32)n << 16) / d;
}
#else
#include "gte_divider.h"
#endif // GTE_USE_NATIVE_DIVIDE

#ifndef FLAGLESS

const unsigned char gte_cycletab[64] = {
	/*   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f */
	 0, 15,  0,  0,  0,  0,  8,  0,  0,  0,  0,  0,  6,  0,  0,  0,
	 8,  8,  8, 19, 13,  0, 44,  0,  0,  0,  0, 17, 11,  0, 14,  0,
	30,  0,  0,  0,  0,  0,  0,  0,  5,  8, 17,  0,  0,  5,  6,  0,
	23,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  5, 39,
};

// warning: ari64 drc stores its negative cycles in gteBusyCycle
static int gteCheckStallRaw(u32 op_cycles, psxRegisters *regs) {
	u32 left = regs->gteBusyCycle - regs->cycle;
	int stall = 0;

	if (left <= 44) {
		//printf("c %2u stall %2u %u\n", op_cycles, left, regs->cycle);
		regs->cycle = regs->gteBusyCycle;
		stall = left;
	}
	regs->gteBusyCycle = regs->cycle + op_cycles;
	return stall;
}

void gteCheckStall(u32 op) {
	gteCheckStallRaw(gte_cycletab[op], &psxRegs);
}

static inline u32 ir2rgb(s32 ir)
{
	ir >>= 7;
	if (ir < 0)
		ir = 0;
	else if (ir > 0x1f)
		ir = 0x1f;
	return ir;
}

/* defined once: the flagless twin of this file (gte_nf.c) includes it
 * with FLAGLESS set and must not duplicate them */
int gte_tag_on;
int gte_tag_null;
u32 gte_tag_h = 1000;      /* the projection distance, for the consumer */

u32 MFC2(struct psxCP2Regs *regs, int reg) {
	switch (reg) {
		case 1:
		case 3:
		case 5:
		case 8:
		case 9:
		case 10:
		case 11:
			regs->CP2D.r[reg] = (s32)regs->CP2D.p[reg].sw.l;
			break;

		case 7:
		case 16:
		case 17:
		case 18:
		case 19:
			regs->CP2D.r[reg] = (u32)regs->CP2D.p[reg].w.l;
			break;

		case 15:
			regs->CP2D.r[reg] = gteSXY2;
			break;

		case 28:
		case 29:
			regs->CP2D.r[reg] = ir2rgb(gteIR1) | (ir2rgb(gteIR2) << 5) | (ir2rgb(gteIR3) << 10);
			break;
	}
	return regs->CP2D.r[reg];
}

static u32 lzc(s32 val)
{
#if __has_builtin(__builtin_clrsb)
	return 1 + __builtin_clrsb(val);
#else
	val ^= val >> 31;
	return val ? __builtin_clz(val) : 32;
#endif
}

void MTC2(struct psxCP2Regs *regs, u32 value, int reg) {
	switch (reg) {
		case 15:
			gteSXY0 = gteSXY1;
			gteSXY1 = gteSXY2;
			gteSXY2 = value;
			gteSXYP = value;
			break;

		case 28:
			gteIRGB = value;
			// not gteIR1 etc. just to be consistent with dynarec
			regs->CP2D.n.ir1 = (value & 0x1f) << 7;
			regs->CP2D.n.ir2 = (value & 0x3e0) << 2;
			regs->CP2D.n.ir3 = (value & 0x7c00) >> 3;
			break;

		case 30:
			gteLZCS = value;
			gteLZCR = lzc(value);
			break;

		case 31:
			return;

		default:
			regs->CP2D.r[reg] = value;
	}
}

void CTC2(struct psxCP2Regs *regs, u32 value, int reg) {
	switch (reg) {
		case 4:
		case 12:
		case 20:
		case 26:
		case 27:
		case 29:
		case 30:
			value = (s32)(s16)value;
			break;

		case 31:
			value = getFinalFlag(value & 0x7ffff000);
			break;
	}

	regs->CP2C.r[reg] = value;
}

#endif // FLAGLESS

#if 0
#define DIVIDE DIVIDE_
static u32 DIVIDE_(s16 n, u16 d) {
	s32 n_ = n;
	return ((n_ << 16) + d / 2) / d;
	//return (u32)((float)(n_ << 16) / (float)d + (float)0.5);
}
#endif

static inline s32 divide(u32 *flags, u16 h, u16 sz3)
{
	if (likely(h < sz3 * 2u)) {
		s32 r = DIVIDE(h, sz3);
		return r >= 0x1ffff ? 0x1ffff : r;
	}
#ifndef FLAGLESS
	*flags |= 1u << 17;
#endif
	return 0x1ffff;
}

#ifdef HAVE_ARMV5

#define gteMAC123f gteMAC123f_arm

#else

static u32 gteMAC123f(psxCP2Regs *regs, s32 vx, s32 vy, s32 vz,
	const s16 *mx, const s32 *cv, int shift)
{
	u32 flags = 0;

	gteMAC1 = mac123add4(1, &flags, cv[0], mx[0] * vx, mx[1] * vy, mx[2] * vz, shift);
	gteMAC2 = mac123add4(2, &flags, cv[1], mx[3] * vx, mx[4] * vy, mx[5] * vz, shift);
	gteMAC3 = mac123add4(3, &flags, cv[2], mx[6] * vx, mx[7] * vy, mx[8] * vz, shift);

	return flags;
}

#endif

/* ---- PGXP-style geometry capture ----------------------------------
 * RTPS/RTPT compute the projected screen position in 16.16 fixed point
 * and then throw the fraction away (the ">> 16" below). That discarded
 * fraction, plus the view depth sz3, is exactly the 3D information the
 * GPU never sees -- the PS1 hands its rasterizer flat 2D triangles.
 * We keep it, keyed by the packed SXY word the game will copy into the
 * GPU packet, so the renderer can look it up when the primitive
 * arrives. Value-keyed on purpose: the alternative (shadowing every
 * store, as stock PGXP does) would mean instrumenting the dynarec.
 * Defined once -- gte_nf.c re-includes this file with FLAGLESS set. */
extern int pgxp_capture_on;
extern void pgxp_note(s64 fx, s64 fy, s32 sx, s32 sy, s32 sz,
                      s32 vx, s32 vy, s32 vz,
                      s32 ofx, s32 ofy, s32 h, int slot);

static inline force_inline void gteRTPS(psxCP2Regs *regs, int shift, int lm)
{
	s32 vx = gteVX0, vy = gteVY0, vz = gteVZ0;
	s32 sz3, quotient;
	s32 mac1, mac2;
	s64 mac3, mac0;
	u32 flags = 0;

	GTE_LOG("GTE RTPS\n");

	gteMAC1 = mac1 = mac123add4(1, &flags, gteTRX, gteR11 * vx, gteR12 * vy, gteR13 * vz, shift);
	gteMAC2 = mac2 = mac123add4(2, &flags, gteTRY, gteR21 * vx, gteR22 * vy, gteR23 * vz, shift);
	gteMAC3 = mac3 = mac123add4(3, &flags, gteTRZ, gteR31 * vx, gteR32 * vy, gteR33 * vz, shift);
	gteIR1 = limB1(&flags, mac1, lm);
	gteIR2 = limB2(&flags, mac2, lm);
	gteIR3 = LIM(&flags, mac3, 0x7fff, -0x8000 * !lm, 0);
	sz3 = mac3 >> (12-shift);
	limBF3(&flags, sz3, 0);
	sz3 = limD(&flags, sz3);
	quotient = divide(&flags, gteH, sz3);
	gteSZ0 = gteSZ1;
	gteSZ1 = gteSZ2;
	gteSZ2 = gteSZ3;
	gteSZ3 = sz3;
	gteSXY0 = gteSXY1;
	gteSXY1 = gteSXY2;

	{
		s64 fx = mac0flags(&flags, gteOFX + (s64)gteIR1 * quotient);
		s64 fy = mac0flags(&flags, gteOFY + (s64)gteIR2 * quotient);
		gteSX2 = limG1(&flags, fx >> 16);
		gteSY2 = limG2(&flags, fy >> 16);
		if (gte_tag_on) {
			gteSXY2 = gte_tag_encode(gteSXY2, (u32)sz3);
			gte_tag_h = gteH;
		}
		if (pgxp_capture_on)   /* mac1..3 = R*V + TR = view space */
			/* IR1/IR2/sz3 are exactly what the projection below
			 * consumes: IR is MAC clamped to +-32767, and sz3 is
			 * mac3 >> (12-shift). Capturing mac1..3 instead put
			 * the re-projection out by up to 686 pixels. */
			pgxp_note(fx, fy, gteSX2, gteSY2, sz3,
			          gteIR1, gteIR2, sz3,
			          gteOFX, gteOFY, gteH, 3);   /* RTPS: PUSH */
	}

	gteMAC0 = mac0 = mac0flags(&flags, gteDQB + (s64)gteDQA * quotient);
	gteIR0 = limH(&flags, mac0 >> 12);
	gteFLAG = getFinalFlag(flags);
}

static inline force_inline void gteRTPT(psxCP2Regs *regs, int shift, int lm)
{
	s32 sz3, quotient;
	s32 mac1, mac2;
	s64 mac3, mac0;
	s32 vx, vy, vz;
	s32 ir1, ir2;
	u32 h = gteH;
	u32 flags = 0;
	int v;

	GTE_LOG("GTE RTPT\n");

	gteSZ0 = gteSZ3;
	for (v = 0; v < 3; v++) {
		vx = VX(v);
		vy = VY(v);
		vz = VZ(v);
		mac1 = mac123add4(1, &flags, gteTRX, gteR11 * vx, gteR12 * vy, gteR13 * vz, shift);
		mac2 = mac123add4(2, &flags, gteTRY, gteR21 * vx, gteR22 * vy, gteR23 * vz, shift);
		mac3 = mac123add4(3, &flags, gteTRZ, gteR31 * vx, gteR32 * vy, gteR33 * vz, shift);
		ir1 = limB1(&flags, mac1, lm);
		ir2 = limB2(&flags, mac2, lm);
		sz3 = mac3 >> (12-shift);
		limBF3(&flags, sz3, 0);
		sz3 = limD(&flags, sz3);
		quotient = divide(&flags, h, sz3);
		fSZ(v) = sz3;
		{
			s64 fx = mac0flags(&flags, gteOFX + (s64)ir1 * quotient);
			s64 fy = mac0flags(&flags, gteOFY + (s64)ir2 * quotient);
			fSX(v) = limG1(&flags, fx >> 16);
			fSY(v) = limG2(&flags, fy >> 16);
			if (gte_tag_on) {
				regs->CP2D.r[12 + v] =
					gte_tag_encode(regs->CP2D.r[12 + v], (u32)sz3);
				gte_tag_h = h;
			}
			if (pgxp_capture_on)
				pgxp_note(fx, fy, fSX(v), fSY(v), sz3,
				          ir1, ir2, sz3,
				          gteOFX, gteOFY, h, v);  /* RTPT -> SXY0..2 */
		}
	}

	gteMAC1 = mac1;
	gteMAC2 = mac2;
	gteMAC3 = mac3;
	gteIR1 = ir1;
	gteIR2 = ir2;
	gteIR3 = LIM(&flags, mac3, 0x7fff, -0x8000 * !lm, 0);
	gteMAC0 = mac0 = mac0flags(&flags, gteDQB + (s64)gteDQA * quotient);
	gteIR0 = limH(&flags, mac0 >> 12);
	gteFLAG = getFinalFlag(flags);
}

static inline force_inline void NM(gteMVMVAn)(psxCP2Regs *regs,
	const s16 *mx, const s16 *v, const s32 *cv, int shift, int lm)
{
	u32 flags = 0;

	flags = gteMAC123f(regs, v[0], v[1], v[2], mx, cv, shift);
	gteIR1 = limB1(&flags, gteMAC1, lm);
	gteIR2 = limB2(&flags, gteMAC2, lm);
	gteIR3 = limB3(&flags, gteMAC3, lm);
	gteFLAG = getFinalFlag(flags);
}

static noinline void NM(gteMVMVAbugged)(psxCP2Regs *regs,
	const s16 *mx, const s16 *v, const s32 *cv, int shift, int lm)
{
	s32 vx = v[0], vy = v[1], vz = v[2];
	u32 flags = 0;
	s32 mac1 = mac123add_s12(1, &flags, cv[0], mx[0] * vx, shift);
	s32 mac2 = mac123add_s12(2, &flags, cv[1], mx[3] * vx, shift);
	s32 mac3 = mac123add_s12(3, &flags, cv[2], mx[6] * vx, shift);
	limBF1(&flags, mac1, 0);
	limBF2(&flags, mac2, 0);
	limBF3(&flags, mac3, 0);
	gteMAC1 = ((s64)(mx[1] * vy) + (mx[2] * vz)) >> shift;
	gteMAC2 = ((s64)(mx[4] * vy) + (mx[5] * vz)) >> shift;
	gteMAC3 = ((s64)(mx[7] * vy) + (mx[8] * vz)) >> shift;
	gteIR1 = limB1(&flags, gteMAC1, lm);
	gteIR2 = limB2(&flags, gteMAC2, lm);
	gteIR3 = limB3(&flags, gteMAC3, lm);
	gteFLAG = getFinalFlag(flags);
}

static noinline void NM(gteMVMVAn_sf0lm0)(psxCP2Regs *regs,
			const s16 *mx, const s16 *v, const s32 *cv) {
	NM(gteMVMVAn)(regs, mx, v, cv,  0, 0);
}
static noinline void NM(gteMVMVAn_sf0lm1)(psxCP2Regs *regs,
			const s16 *mx, const s16 *v, const s32 *cv) {
	NM(gteMVMVAn)(regs, mx, v, cv,  0, 1);
}
static noinline void NM(gteMVMVAn_sf1lm0)(psxCP2Regs *regs,
			const s16 *mx, const s16 *v, const s32 *cv) {
	NM(gteMVMVAn)(regs, mx, v, cv, 12, 0);
}
static noinline void NM(gteMVMVAn_sf1lm1)(psxCP2Regs *regs,
			const s16 *mx, const s16 *v, const s32 *cv) {
	NM(gteMVMVAn)(regs, mx, v, cv, 12, 1);
}

static inline force_inline void gteMVMVA(psxCP2Regs *regs,
	int mx, int v, int cv, int shift, int lm)
{
	const s16 v3[3] = { regs->CP2D.p[9].sw.l, regs->CP2D.p[10].sw.l, regs->CP2D.p[11].sw.l };
	const s16 *vp = &regs->CP2D.p[v << 1].sw.l;
	const s16 *mxp = &regs->CP2C.p[mx << 3].sw.l;
	const s32 *cvp = (s32 *)&regs->CP2C.r[(cv << 3) + 5];
	const s32 cv3[3] = { 0, 0, 0 };
	s16 mx3[9];

	GTE_LOG("GTE MVMVA\n");

	if (v == 3)
		vp = v3;
	if (unlikely(mx == 3)) {
		mxp = mx3;
		mx3[0] = -regs->CP2D.p[6].b.l << 4;
		mx3[1] =  regs->CP2D.p[6].b.l << 4;
		mx3[2] = regs->CP2D.p[8].sw.l;
		mx3[3] = regs->CP2C.p[1].sw.l;
		mx3[4] = regs->CP2C.p[1].sw.l;
		mx3[5] = regs->CP2C.p[1].sw.l;
		mx3[6] = regs->CP2C.p[2].sw.l;
		mx3[7] = regs->CP2C.p[2].sw.l;
		mx3[8] = regs->CP2C.p[2].sw.l;
	}
	if (unlikely(cv == 3))
		cvp = cv3;
	if (unlikely(cv == 2))
		NM(gteMVMVAbugged)(regs, mxp, vp, cvp, shift, lm);
	else {
		if (shift && lm)
			NM(gteMVMVAn_sf1lm1)(regs, mxp, vp, cvp);
		else if (shift && !lm)
			NM(gteMVMVAn_sf1lm0)(regs, mxp, vp, cvp);
		else if (lm)
			NM(gteMVMVAn_sf0lm1)(regs, mxp, vp, cvp);
		else
			NM(gteMVMVAn_sf0lm0)(regs, mxp, vp, cvp);
	}
}

void NM(gteMVMVA_generic)(psxCP2Regs *regs, u32 code)
{
	gteMVMVA(regs, GTE_MX(code), GTE_V(code), GTE_CV(code),
	         12 * GTE_SF(code), GTE_LM(code));
}

static inline void gteNCLIP_(psxCP2Regs *regs)
{
	u32 flags = 0;

	GTE_LOG("GTE NCLIP\n");

	if (gte_tag_on) {
		/* the top five bits of each coordinate carry the depth tag, and
		 * back-face culling must not see them: sign-extend from bit 10,
		 * which is the whole range limG1/limG2 can produce anyway */
		s32 x0 = tag11(gteSX0), y0 = tag11(gteSY0);
		s32 x1 = tag11(gteSX1), y1 = tag11(gteSY1);
		s32 x2 = tag11(gteSX2), y2 = tag11(gteSY2);
		gteMAC0 = mac0flags(&flags, (s64)(x0 * (y1 - y2)) +
					x1 * (y2 - y0) +
					x2 * (y0 - y1));
		gteFLAG = getFinalFlag(flags);
		return;
	}
	gteMAC0 = mac0flags(&flags, (s64)(gteSX0 * (gteSY1 - gteSY2)) +
				gteSX1 * (gteSY2 - gteSY0) +
				gteSX2 * (gteSY0 - gteSY1));
	gteFLAG = getFinalFlag(flags);
}

static inline void gteAVSZ3_(psxCP2Regs *regs)
{
	u32 flags = 0;
	s64 r;

	GTE_LOG("GTE AVSZ3\n");

	r = (s64)gteZSF3 * (gteSZ1 + gteSZ2 + gteSZ3);
	gteMAC0 = mac0flags(&flags, r);
	gteOTZ = limD(&flags, r >> 12);
	gteFLAG = getFinalFlag(flags);
}

static inline void gteAVSZ4_(psxCP2Regs *regs)
{
	u32 flags = 0;
	s64 r;

	GTE_LOG("GTE AVSZ4\n");

	r = (s64)gteZSF4 * (gteSZ0 + gteSZ1 + gteSZ2 + gteSZ3);
	gteMAC0 = mac0flags(&flags, r);
	gteOTZ = limD(&flags, r >> 12);
	gteFLAG = getFinalFlag(flags);
}

static inline void gteSQR(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE SQR\n");

	gteMAC1 = (gteIR1 * gteIR1) >> shift;
	gteMAC2 = (gteIR2 * gteIR2) >> shift;
	gteMAC3 = (gteIR3 * gteIR3) >> shift;
	gteIR1 = limB1(&flags, gteMAC1, lm);
	gteIR2 = limB2(&flags, gteMAC2, lm);
	gteIR3 = limB3(&flags, gteMAC3, lm);
	gteFLAG = getFinalFlag(flags);
}

static inline force_inline void runColor1(psxCP2Regs *regs, u32 *flags, int shift, int lm,
	int is_ncc, int is_ncd, int v, int o, int writeback)
{
	s32 mac1, mac2, mac3;
	s32 ir1, ir2, ir3;
	s32 vx, vy, vz;

	vx = VX(v);
	vy = VY(v);
	vz = VZ(v);
	mac1 = ((s64)(gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> shift;
	mac2 = ((s64)(gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> shift;
	mac3 = ((s64)(gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> shift;
	ir1 = limB1(flags, mac1, lm);
	ir2 = limB2(flags, mac2, lm);
	ir3 = limB3(flags, mac3, lm);
	*flags |= gteMAC123f(regs, ir1, ir2, ir3, &gteLR1, &gteRBK, shift);
	mac1 = gteMAC1;
	mac2 = gteMAC2;
	mac3 = gteMAC3;
	if (is_ncc || is_ncd) {
		ir1 = limB1(flags, mac1, lm);
		ir2 = limB2(flags, mac2, lm);
		ir3 = limB3(flags, mac3, lm);
		mac1 = ((s32)gteR * ir1) << 4;
		mac2 = ((s32)gteG * ir2) << 4;
		mac3 = ((s32)gteB * ir3) << 4;
		if (is_ncd) {
			s32 ir0 = gteIR0;
			ir1 = limB1(flags, mac123sub_s12(1, flags, gteRFC, mac1, shift), 0);
			ir2 = limB2(flags, mac123sub_s12(2, flags, gteGFC, mac2, shift), 0);
			ir3 = limB3(flags, mac123sub_s12(3, flags, gteBFC, mac3, shift), 0);
			mac1 = (ir0 * ir1 + (s64)mac1) >> shift;
			mac2 = (ir0 * ir2 + (s64)mac2) >> shift;
			mac3 = (ir0 * ir3 + (s64)mac3) >> shift;
		}
		else {
			mac1 >>= shift;
			mac2 >>= shift;
			mac3 >>= shift;
		}
	}
	fR(o) = limC1(flags, mac1 >> 4);
	fG(o) = limC2(flags, mac2 >> 4);
	fB(o) = limC3(flags, mac3 >> 4);
	fCODE(o) = gteCODE;
	if (writeback) {
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
		gteIR1 = limB1(flags, mac1, lm);
		gteIR2 = limB2(flags, mac2, lm);
		gteIR3 = limB3(flags, mac3, lm);
		gteFLAG = getFinalFlag(*flags);
	}
	else {
		limBF1(flags, mac1, lm);
		limBF2(flags, mac2, lm);
		limBF3(flags, mac3, lm);
	}
}

static inline void shiftRGB(psxCP2Regs *regs)
{
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
}

static inline void gteNCCS(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE NCCS\n");

	shiftRGB(regs);
	runColor1(regs, &flags, shift, lm, 1, 0, 0, 2, 1);
}

static inline void gteNCCT(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE NCCT\n");

	runColor1(regs, &flags, shift, lm, 1, 0, 0, 0, 0);
	runColor1(regs, &flags, shift, lm, 1, 0, 1, 1, 0);
	runColor1(regs, &flags, shift, lm, 1, 0, 2, 2, 1);
}

static inline void gteNCDS(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE NCDS\n");

	shiftRGB(regs);
	runColor1(regs, &flags, shift, lm, 0, 1, 0, 2, 1);
}

static inline void gteNCDT(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE NCDT\n");

	runColor1(regs, &flags, shift, lm, 0, 1, 0, 0, 0);
	runColor1(regs, &flags, shift, lm, 0, 1, 1, 1, 0);
	runColor1(regs, &flags, shift, lm, 0, 1, 2, 2, 1);
}

static inline void gteNCS(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE NCS\n");

	shiftRGB(regs);
	runColor1(regs, &flags, shift, lm, 0, 0, 0, 2, 1);
}

static inline void gteNCT(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE NCT\n");

	runColor1(regs, &flags, shift, lm, 0, 0, 0, 0, 0);
	runColor1(regs, &flags, shift, lm, 0, 0, 1, 1, 0);
	runColor1(regs, &flags, shift, lm, 0, 0, 2, 2, 1);
}

static inline void gteOP(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE OP\n");

	gteMAC1 = ((gteR22 * gteIR3) - (gteR33 * gteIR2)) >> shift;
	gteMAC2 = ((gteR33 * gteIR1) - (gteR11 * gteIR3)) >> shift;
	gteMAC3 = ((gteR11 * gteIR2) - (gteR22 * gteIR1)) >> shift;
	gteIR1 = limB1(&flags, gteMAC1, lm);
	gteIR2 = limB2(&flags, gteMAC2, lm);
	gteIR3 = limB3(&flags, gteMAC3, lm);
	gteFLAG = getFinalFlag(flags);
}

static inline void gteGPF(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE GPF\n");

	shiftRGB(regs);
	gteMAC1 = (gteIR0 * gteIR1) >> shift;
	gteMAC2 = (gteIR0 * gteIR2) >> shift;
	gteMAC3 = (gteIR0 * gteIR3) >> shift;
	gteIR1 = limB1(&flags, gteMAC1, lm);
	gteIR2 = limB2(&flags, gteMAC2, lm);
	gteIR3 = limB3(&flags, gteMAC3, lm);
	gteR2 = limC1(&flags, gteMAC1 >> 4);
	gteG2 = limC2(&flags, gteMAC2 >> 4);
	gteB2 = limC3(&flags, gteMAC3 >> 4);
	gteFLAG = getFinalFlag(flags);
}

static inline void gteGPL(psxCP2Regs *regs, int shift, int lm)
{
	s16 ir0 = gteIR0;
	u32 flags = 0;

	GTE_LOG("GTE GPL\n");

	shiftRGB(regs);
	if (shift) {
		gteMAC1 = mac123add_s12(1, &flags, gteMAC1, ir0 * gteIR1, 12);
		gteMAC2 = mac123add_s12(2, &flags, gteMAC2, ir0 * gteIR2, 12);
		gteMAC3 = mac123add_s12(3, &flags, gteMAC3, ir0 * gteIR3, 12);
	}
	else {
		gteMAC1 += ir0 * gteIR1;
		gteMAC2 += ir0 * gteIR2;
		gteMAC3 += ir0 * gteIR3;
	}
	gteIR1 = limB1(&flags, gteMAC1, lm);
	gteIR2 = limB2(&flags, gteMAC2, lm);
	gteIR3 = limB3(&flags, gteMAC3, lm);
	gteR2 = limC1(&flags, gteMAC1 >> 4);
	gteG2 = limC2(&flags, gteMAC2 >> 4);
	gteB2 = limC3(&flags, gteMAC3 >> 4);
	gteFLAG = getFinalFlag(flags);
}

static inline void runColor2(psxCP2Regs *regs, u32 *flags, int shift, int lm,
	s32 mac1, s32 mac2, s32 mac3, int o, int writeback)
{
	s32 ir1, ir2, ir3;
	s32 ir0 = gteIR0;

	ir1 = limB1(flags, mac123sub_s12(1, flags, gteRFC, mac1, shift), 0);
	ir2 = limB2(flags, mac123sub_s12(2, flags, gteGFC, mac2, shift), 0);
	ir3 = limB3(flags, mac123sub_s12(3, flags, gteBFC, mac3, shift), 0);
	mac1 = gteMAC1 = (ir0 * ir1 + (s64)mac1) >> shift;
	mac2 = gteMAC2 = (ir0 * ir2 + (s64)mac2) >> shift;
	mac3 = gteMAC3 = (ir0 * ir3 + (s64)mac3) >> shift;
	ir1 = limB1(flags, mac1, lm);
	ir2 = limB2(flags, mac2, lm);
	ir3 = limB3(flags, mac3, lm);
	fR(o) = limC1(flags, mac1 >> 4);
	fG(o) = limC2(flags, mac2 >> 4);
	fB(o) = limC3(flags, mac3 >> 4);
	if (writeback) {
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
		gteIR1 = ir1;
		gteIR2 = ir2;
		gteIR3 = ir3;
		gteFLAG = getFinalFlag(*flags);
	}
}

static inline void gteDCPL(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE DCPL\n");

	s32 mac1 = ((s32)gteR * gteIR1) << 4;
	s32 mac2 = ((s32)gteG * gteIR2) << 4;
	s32 mac3 = ((s32)gteB * gteIR3) << 4;
	shiftRGB(regs);
	runColor2(regs, &flags, shift, lm, mac1, mac2, mac3, 2, 1);
}

static inline void gteDPCS(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE DPCS\n");

	shiftRGB(regs);
	runColor2(regs, &flags, shift, lm, gteR << 16, gteG << 16, gteB << 16, 2, 1);
}

static inline void gteDPCT(psxCP2Regs *regs, int shift, int lm)
{
	s32 mac1, mac2, mac3;
	u8 code0 = gteCODE;
	u32 flags = 0;
	int v;

	GTE_LOG("GTE DPCT\n");

	for (v = 0; v < 3; v++) {
		mac1 = fR(v) << 16;
		mac2 = fG(v) << 16;
		mac3 = fB(v) << 16;
		runColor2(regs, &flags, shift, lm, mac1, mac2, mac3, v, v == 2);
		fCODE(v) = code0;
	}
}

static inline void gteINTPL(psxCP2Regs *regs, int shift, int lm)
{
	u32 flags = 0;

	GTE_LOG("GTE INTPL\n");

	shiftRGB(regs);
	runColor2(regs, &flags, shift, lm, gteIR1 << 12, gteIR2 << 12, gteIR3 << 12, 2, 1);
}

static inline void runColor3(psxCP2Regs *regs, int shift, int lm, int is_cdp)
{
	s32 mac1, mac2, mac3;
	s32 ir1, ir2, ir3;
	u32 flags;

	flags = gteMAC123f(regs, gteIR1, gteIR2, gteIR3, &gteLR1, &gteRBK, shift);
	ir1 = limB1(&flags, gteMAC1, lm);
	ir2 = limB2(&flags, gteMAC2, lm);
	ir3 = limB3(&flags, gteMAC3, lm);
	mac1 = ((s32)gteR * ir1) << 4;
	mac2 = ((s32)gteG * ir2) << 4;
	mac3 = ((s32)gteB * ir3) << 4;
	if (is_cdp) {
		s32 ir0 = gteIR0;
		ir1 = limB1(&flags, mac123sub_s12(1, &flags, gteRFC, mac1, shift), 0);
		ir2 = limB2(&flags, mac123sub_s12(2, &flags, gteGFC, mac2, shift), 0);
		ir3 = limB3(&flags, mac123sub_s12(3, &flags, gteBFC, mac3, shift), 0);
		mac1 = (ir0 * ir1 + (s64)mac1) >> shift;
		mac2 = (ir0 * ir2 + (s64)mac2) >> shift;
		mac3 = (ir0 * ir3 + (s64)mac3) >> shift;
	}
	else {
		mac1 >>= shift;
		mac2 >>= shift;
		mac3 >>= shift;
	}
	gteMAC1 = mac1;
	gteMAC2 = mac2;
	gteMAC3 = mac3;
	gteIR1 = limB1(&flags, mac1, lm);
	gteIR2 = limB2(&flags, mac2, lm);
	gteIR3 = limB3(&flags, mac3, lm);
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(&flags, mac1 >> 4);
	gteG2 = limC2(&flags, mac2 >> 4);
	gteB2 = limC3(&flags, mac3 >> 4);
	gteFLAG = getFinalFlag(flags);
}

static inline void gteCC(psxCP2Regs *regs, int shift, int lm)
{
	GTE_LOG("GTE CC\n");
	runColor3(regs, shift, lm, 0);
}

static inline void gteCDP(psxCP2Regs *regs, int shift, int lm)
{
	GTE_LOG("GTE CDP\n");
	runColor3(regs, shift, lm, 1);
}

#define DECL_OP4__(name, nf, decl_) \
decl_ void gte##name##_sf0lm0##nf(psxCP2Regs *regs, u32 code) { gte##name(regs,  0, 0); } \
decl_ void gte##name##_sf0lm1##nf(psxCP2Regs *regs, u32 code) { gte##name(regs,  0, 1); } \
decl_ void gte##name##_sf1lm0##nf(psxCP2Regs *regs, u32 code) { gte##name(regs, 12, 0); } \
decl_ void gte##name##_sf1lm1##nf(psxCP2Regs *regs, u32 code) { gte##name(regs, 12, 1); }
#define DECL_OP1_(name, nf) \
static void gte##name##nf(psxCP2Regs *regs, u32 code) { gte##name##_(regs); }

#define DECL_OP4_(name, nf, decl_) DECL_OP4__(name, nf, decl_)
#define DECL_OP4(name, nf) DECL_OP4_(name, nf, static)
#define DECL_OP1(name, nf) DECL_OP1_(name, nf)

#define DECL_MVMVA1(nf, sf_, mx_, v_, cv_, lm_) \
static void no_stackprotector gteMVMVA_mx##mx_##v##v_##cv##cv_##sf##sf_##lm##lm_##nf( \
		psxCP2Regs *regs, u32 code) { \
	gteMVMVA(regs, mx_, v_, cv_, sf_ * 12, lm_); \
}
#define DECL_MVMVA2( nf, sf, mx, v, cv) \
	DECL_MVMVA1( nf, sf, mx, v, cv, 0) \
	DECL_MVMVA1( nf, sf, mx, v, cv, 1)
#define DECL_MVMVA4( nf, sf, mx, v) \
	DECL_MVMVA2( nf, sf, mx, v, 0) \
	DECL_MVMVA2( nf, sf, mx, v, 1)
#define DECL_MVMVA16(nf, sf, mx) \
	DECL_MVMVA4( nf, sf, mx, 0) \
	DECL_MVMVA4( nf, sf, mx, 1) \
	DECL_MVMVA4( nf, sf, mx, 2) \
	DECL_MVMVA4( nf, sf, mx, 3)
#define DECL_MVMVA32(nf, sf) \
	DECL_MVMVA16(nf, sf, 0) \
	DECL_MVMVA16(nf, sf, 1)
#define DECL_MVMVA64(nf) \
	DECL_MVMVA32(nf, 0) \
	DECL_MVMVA32(nf, 1)

#define TABLE_ENTRY4(name) \
	[GTEOP_##name * 4 + 0] = NM(gte##name##_sf0lm0), \
	[GTEOP_##name * 4 + 1] = NM(gte##name##_sf0lm1), \
	[GTEOP_##name * 4 + 2] = NM(gte##name##_sf1lm0), \
	[GTEOP_##name * 4 + 3] = NM(gte##name##_sf1lm1)
#define TABLE_ENTRY1(name) \
	[GTEOP_##name * 4 + 0] = NM(gte##name), \
	[GTEOP_##name * 4 + 1] = NM(gte##name), \
	[GTEOP_##name * 4 + 2] = NM(gte##name), \
	[GTEOP_##name * 4 + 3] = NM(gte##name)
#define TABLE_ENTRY1_MVMVA1(nf, sf_, mx_, v_, cv_, lm_) \
	[0x40*4 + sf_*32 + mx_*16 + v_*4 + cv_*2 + lm_] = \
		gteMVMVA_mx##mx_##v##v_##cv##cv_##sf##sf_##lm##lm_##nf,
#define TABLE_ENTRY1_MVMVA2( nf, sf, mx, v, cv) \
	TABLE_ENTRY1_MVMVA1( nf, sf, mx, v, cv, 0) \
	TABLE_ENTRY1_MVMVA1( nf, sf, mx, v, cv, 1)
#define TABLE_ENTRY1_MVMVA4( nf, sf, mx, v) \
	TABLE_ENTRY1_MVMVA2( nf, sf, mx, v, 0) \
	TABLE_ENTRY1_MVMVA2( nf, sf, mx, v, 1)
#define TABLE_ENTRY1_MVMVA16(nf, sf, mx) \
	TABLE_ENTRY1_MVMVA4( nf, sf, mx, 0) \
	TABLE_ENTRY1_MVMVA4( nf, sf, mx, 1) \
	TABLE_ENTRY1_MVMVA4( nf, sf, mx, 2) \
	TABLE_ENTRY1_MVMVA4( nf, sf, mx, 3)
#define TABLE_ENTRY1_MVMVA32(nf, sf) \
	TABLE_ENTRY1_MVMVA16(nf, sf, 0) \
	TABLE_ENTRY1_MVMVA16(nf, sf, 1)
#define TABLE_ENTRY1_MVMVA64(nf) \
	TABLE_ENTRY1_MVMVA32(nf, 0) \
	TABLE_ENTRY1_MVMVA32(nf, 1)

DECL_OP4_(RTPS, NF, )
DECL_OP1(NCLIP, NF)
DECL_OP4(OP,    NF)
DECL_OP4(DPCS,  NF)
DECL_OP4(INTPL, NF)
DECL_OP4(NCDS,  NF)
DECL_OP4(CDP,   NF)
DECL_OP4(NCDT,  NF)
DECL_OP4(NCCS,  NF)
DECL_OP4(CC,    NF)
DECL_OP4(NCS,   NF)
DECL_OP4(NCT,   NF)
DECL_OP4(SQR,   NF)
DECL_OP4(DCPL,  NF)
DECL_OP4(DPCT,  NF)
DECL_OP1(AVSZ3, NF)
DECL_OP1(AVSZ4, NF)
DECL_OP4_(RTPT, NF, )
DECL_OP4(GPF,   NF)
DECL_OP4(GPL,   NF)
DECL_OP4(NCCT,  NF)
DECL_MVMVA64(NF)

static void (*gteOPS[0x40*(4+1)])(psxCP2Regs *regs, u32 code) =
{
	TABLE_ENTRY4(RTPS),
	TABLE_ENTRY1(NCLIP),
	TABLE_ENTRY4(OP),
	TABLE_ENTRY4(DPCS),
	TABLE_ENTRY4(INTPL),
	//TABLE_ENTRY1(MVMVA),
	TABLE_ENTRY4(NCDS),
	TABLE_ENTRY4(CDP),
	TABLE_ENTRY4(NCDT),
	TABLE_ENTRY4(NCCS),
	TABLE_ENTRY4(CC),
	TABLE_ENTRY4(NCS),
	TABLE_ENTRY4(NCT),
	TABLE_ENTRY4(SQR),
	TABLE_ENTRY4(DCPL),
	TABLE_ENTRY4(DPCT),
	TABLE_ENTRY1(AVSZ3),
	TABLE_ENTRY1(AVSZ4),
	TABLE_ENTRY4(RTPT),
	TABLE_ENTRY4(GPF),
	TABLE_ENTRY4(GPL),
	TABLE_ENTRY4(NCCT),
	TABLE_ENTRY1_MVMVA64(NF)
};

gte_handler * NM(gteGetHandler)(u32 code)
{
	u32 l, op = code & 0x3f;
	u32 sflm = ((code >> (19-1)) & 2) | ((code >> 10) & 1);
	gte_handler *h = gteOPS[(op << 2) | sflm];
	if (h)
		return h;
	if (op == GTEOP_MVMVA) {
		if (code & ((1u << 18) | (1u << 14))) // "bugged" mx/cv
			h = NM(gteMVMVA_generic);
		else {
			l = ((code >> (19-5)) & 0x20) | // sf
			    ((code >> (17-4)) & 0x10) | // mx
			    ((code >> (15-2)) & 0x0c) | // v
			    ((code >> (13-1)) & 0x02) | // cv
			    ((code >>  10)    & 0x01);  // lm
			h = gteOPS[0x40*4 + l];
		}
	}
	return h;
}

#ifndef FLAGLESS

/* Direct-mapped, keyed by the 11-bit-per-axis packed XY the GPU packet
 * carries. Two vertices that project to the same integer pixel collide
 * and the newer wins -- harmless, they are at the same place on screen.
 * Sized well above a frame's vertex count so a frame never evicts
 * itself. */
/* 8192 slots evicted hard: with entries living several epochs, a few
 * thousand keys were competing and 59% of all misses were an occupied
 * slot holding SOMEONE ELSE'S key. Not a hash-quality problem alone --
 * the table was simply too small for the working set. */
#define PGXP_N 32768
/* how many epochs (vblanks) an entry stays usable */
#define PGXP_SLACK 4
/* epoch = the frame this transform belongs to; amb = two vertices with
 * DIFFERENT depth landed on the same integer pixel this frame, so the
 * key cannot say which one a packet means. Without these two guards
 * this table is exactly PGXP's "vertex cache", which its own
 * maintainers say produces glitches in most games and should be left
 * off — a stale or ambiguous entry matches by coincidence and hands
 * back a confidently wrong depth. */
/* x,y = precise screen position; z = view depth; vx,vy,vz = the FULL
 * view-space vector the GTE computed before it projected anything.
 * Stock PGXP keeps only the projected values, because its goal is
 * perspective-correct texturing. Ours is stereo, and a real 3D vector
 * lets each eye be projected from its own camera rather than nudged
 * sideways -- so keep both. */
typedef struct {
	u32 key, epoch;
	float x, y, z;
	float vx, vy, vz;
	float ofx, ofy, h;   /* the projection THIS vertex was made with */
	u8 amb;
} pgxp_ent;
static pgxp_ent pgxp_tab[PGXP_N];
static u32 pgxp_epoch = 1;
int pgxp_capture_on;
unsigned pgxp_writes, pgxp_amb, pgxp_stale;
/* the projection the GTE itself used, so a consumer can re-project the
 * view-space vectors exactly rather than guessing a focal length */
unsigned pgxp_m_empty, pgxp_m_key, pgxp_m_amb;

/* called once per frame by the GPU frontend */
/* GTE truth stream: every projection the GTE performs, recorded at
 * computation time -- screen x, y (post-limG, what the packet will
 * carry) and depth. This is the reference no courier bug can touch:
 * decoded vertices are diffed against it offline (EMUCTL 35 arms,
 * sdmc:/psxgpu/truth.bin lands, 3 vblank windows to straddle the
 * game's compute-then-DMA skew). */
int pgxp_truth_req;
#define PGXP_TRUTH_MAX 65536
typedef struct { float x, y, z; } pgxp_truth_rec;
static pgxp_truth_rec *pgxp_truth_buf;
static int pgxp_truth_n = -1;
static int pgxp_truth_frames;

static void pgxp_truth_note(float x, float y, float z)
{
	if (pgxp_truth_n >= 0 && pgxp_truth_n < PGXP_TRUTH_MAX) {
		pgxp_truth_buf[pgxp_truth_n].x = x;
		pgxp_truth_buf[pgxp_truth_n].y = y;
		pgxp_truth_buf[pgxp_truth_n].z = z;
		pgxp_truth_n++;
	}
}

void pgxp_frame(void)
{
	pgxp_epoch++;
	if (pgxp_truth_req && pgxp_truth_n < 0) {
		pgxp_truth_req = 0;
		if (!pgxp_truth_buf)
			pgxp_truth_buf = malloc(PGXP_TRUTH_MAX * sizeof(pgxp_truth_rec));
		if (pgxp_truth_buf) {
			pgxp_truth_n = 0;
			pgxp_truth_frames = 0;
		}
	} else if (pgxp_truth_n >= 0 && ++pgxp_truth_frames >= 3) {
		if (pgxp_truth_n > 0) {
			FILE *f = fopen("sdmc:/psxgpu/truth.bin", "wb");
			if (f) {
				fwrite(pgxp_truth_buf, sizeof(pgxp_truth_rec),
				       pgxp_truth_n, f);
				fclose(f);
			}
		}
		pgxp_truth_n = -1;
	}
}

static u32 pgxp_key(u32 sx, u32 sy)
{
	return (sx & 0x7FF) | ((sy & 0x7FF) << 16);
}

/* Knuth multiplicative. The previous (key ^ (key >> 13)) folded y down
 * onto x and clustered heavily for screen coordinates, which are dense
 * in a small range rather than uniformly distributed. */
static u32 pgxp_slot(u32 key)
{
	return (key * 2654435761u) >> (32 - 15);
}

/* ---- ADDRESS PROVENANCE ------------------------------------------
 * The screen coordinate is a fingerprint: two vertices can share one,
 * which is our single largest source of lost depth. A RAM ADDRESS
 * cannot be shared — the game stores each transform result to its own
 * slot in the display list.
 *
 * So: remember which transform currently sits in each screen-FIFO slot,
 * and when the game drains a slot with SWC2, record the destination
 * address. The GPU side can then match a packet word to the exact
 * transform that produced it, instead of guessing from the pixel.
 *
 * This does NOT need memory shadowing. Full PGXP instruments every load
 * and store; we only need the one instruction that drains the GTE. */
/* A real shadow, one slot per 32-bit word of PS1 RAM, exactly as PGXP
 * does it. The old table was 16384 entries indexed
 * (addr >> 2) & 16383 over a 512K-word RAM, so any two addresses 64 KB
 * apart aliased onto each other.
 *
 * The transform itself is NOT stored inline -- 40 bytes per RAM word
 * would be 21 MB. Instead the shadow holds a sequence number into a
 * ring of the transforms we captured, which costs 8 bytes per word and
 * self-invalidates: once the ring has wrapped past a sequence number,
 * that transform is provably gone and the lookup fails cleanly rather
 * than reading whatever recycled the slot. */
#define PGXP_RAM_WORDS  (2*1024*1024/4)
#define PGXP_POOL_N     65536          /* power of two; ~1 frame of MGS */

/* what a consumer actually needs back; no key/amb, those belong to the
 * screen-keyed table only */
typedef struct {
	u32 epoch;
	float x, y, z;
	float vx, vy, vz;
	float ofx, ofy, h;
} pgxp_xf;

typedef struct { u32 val, seq; } pgxp_shadow_ent;

static pgxp_xf         *pgxp_pool;      /* PGXP_POOL_N, lazily allocated */
static pgxp_shadow_ent *pgxp_shadow;    /* PGXP_RAM_WORDS, ditto */
static u32 pgxp_pool_head = 1;          /* next seq; 0 means "no entry" */
static u32 pgxp_fifo_seq[4];            /* seq of what is in SXY0..2/SXYP */

unsigned pgxp_stores, pgxp_addr_hit, pgxp_addr_miss, pgxp_addr_stale;
unsigned pgxp_addr_evict, pgxp_addr_old, pgxp_addr_pos;
/* The recompiler reads this at block-compile time to decide whether to
 * emit the SWC2 hook, so it must be settled during GPUinit, before any
 * game code is compiled, and never change afterwards. */
int pgxp_addr_on;

/* Called once, from the GPU plugin, when address provenance is enabled.
 * ~6.8 MB, so it is not paid for unless the feature is on. Returns 0 on
 * failure, and the feature simply stays off. */
int pgxp_shadow_alloc(void)
{
	if (pgxp_shadow && pgxp_pool)
		return 1;
	if (!pgxp_shadow)
		pgxp_shadow = calloc(PGXP_RAM_WORDS, sizeof(*pgxp_shadow));
	if (!pgxp_pool)
		pgxp_pool = calloc(PGXP_POOL_N, sizeof(*pgxp_pool));
	if (!pgxp_shadow || !pgxp_pool) {
		free(pgxp_shadow); pgxp_shadow = NULL;
		free(pgxp_pool);   pgxp_pool = NULL;
		return 0;
	}
	pgxp_addr_on = 1;
	return 1;
}

/* RAM only. The 8 MB region at the bottom of each segment mirrors the
 * 2 MB of real RAM; scratchpad (0x1f800000) and I/O must NOT be folded
 * into it, or a scratchpad write would corrupt a RAM slot. */
static int pgxp_ram_word(u32 addr, u32 *word)
{
	u32 pa = addr & 0x1fffffff;
	if (pa >= 0x00800000)
		return 0;
	*word = (pa & 0x1ffffc) >> 2;
	return 1;
}

/* called when the game writes CP2 data register `creg` to `addr` --
 * from the interpreter's gteSWC2 AND from the code the recompiler
 * emits for SWC2 (see c2ls_assemble). Only the screen-coordinate FIFO
 * carries geometry we track. */
void pgxp_store(u32 addr, int creg, u32 val)
{
	u32 w;
	int slot;
	if (!pgxp_shadow)
		return;
	if (creg < 12 || creg > 15)
		return;                    /* not a screen coordinate */
	slot = (creg == 15) ? 2 : creg - 12;   /* SXYP mirrors SXY2 */
	if (!pgxp_fifo_seq[slot])
		return;
	if (!pgxp_ram_word(addr, &w))
		return;
	pgxp_shadow[w].val = val;  /* what memory held when we recorded it */
	pgxp_shadow[w].seq = pgxp_fifo_seq[slot];
	pgxp_stores++;
}

/* ---- MEMORY MODE ------------------------------------------------
 * SWC2 alone recovers almost nothing in practice: measured on MGS,
 * only 3016 of 334980 display-list vertices came from an address the
 * game had written with swc2, against 376485 swc2 stores in the same
 * window. The game drains the GTE into a work buffer and assembles
 * the packets with ordinary loads and stores, so the tag has to
 * survive that copy. That is what PGXP's memory mode is for.
 *
 * A shadow entry per general-purpose register, moved by the same
 * three instructions the data moves through:
 *   mfc2 rt, $12..15  -- a transform enters a register
 *   lw   rt, off(rs)  -- a tagged word enters a register
 *   sw   rt, off(rs)  -- a tagged register lands in memory
 *
 * The load hook deliberately does NOT read the loaded value. It copies
 * the RECORDED value out of the shadow, and the store hook compares
 * that against the value actually being stored. If our record was out
 * of date, or if the register was recomputed in between, the two
 * disagree and the tag is dropped -- which is the same validation rule
 * that protects everything else here, reused to save a second call in
 * the hottest path in the recompiler. */
static pgxp_shadow_ent pgxp_gpr[32];
unsigned pgxp_mloads, pgxp_mstores, pgxp_mdrops;
int pgxp_mem_on;

/* mfc2 rt, $creg: a GTE result enters the general-purpose file. Only
 * the screen-coordinate registers carry geometry we track. */
void pgxp_mfc2(u32 rt, u32 creg)
{
	if (!pgxp_shadow || rt >= 32)
		return;
	if (creg >= 12 && creg <= 15) {
		int slot = (creg == 15) ? 2 : (int)creg - 12;
		pgxp_gpr[rt].seq = pgxp_fifo_seq[slot];
		pgxp_gpr[rt].val = psxRegs.CP2D.r[creg];
	} else {
		pgxp_gpr[rt].seq = 0;
	}
}

void pgxp_mem_load(u32 addr, u32 rt)
{
	u32 w;
	if (!pgxp_shadow || rt >= 32)
		return;
	if (!pgxp_ram_word(addr, &w)) {
		pgxp_gpr[rt].seq = 0;
		return;
	}
	pgxp_gpr[rt] = pgxp_shadow[w];
	if (pgxp_gpr[rt].seq)
		pgxp_mloads++;
}

void pgxp_mem_store(u32 addr, u32 rt, u32 val)
{
	u32 w;
	if (!pgxp_shadow || rt >= 32)
		return;
	if (!pgxp_ram_word(addr, &w))
		return;
	if (pgxp_gpr[rt].seq && pgxp_gpr[rt].val == val) {
		pgxp_shadow[w].val = val;
		pgxp_shadow[w].seq = pgxp_gpr[rt].seq;
		pgxp_mstores++;
	} else if (pgxp_shadow[w].seq) {
		/* untracked data now lives at this address: drop the old tag
		 * rather than leave it to be rejected later, so that a miss
		 * reads as a miss and not as staleness */
		pgxp_shadow[w].seq = 0;
		pgxp_mdrops++;
	}
}

/* memory mode implies the address path; both need the same shadow */
int pgxp_mem_enable(void)
{
	if (!pgxp_shadow_alloc())
		return 0;
	pgxp_mem_on = 1;
	return 1;
}

/* Turn memory mode on or off while the game runs. The recompiler bakes
 * the hooks in when it compiles a block, so nothing changes for code
 * already translated -- the caller has to throw the block cache away
 * for this to take effect, which is why the return value says whether
 * anything actually changed. Cheap enough for a human action like
 * moving the 3D slider; far too expensive to do per frame. */
int pgxp_mem_set(int on)
{
	on = on ? 1 : 0;
	if (!pgxp_shadow && on && !pgxp_shadow_alloc())
		return 0;
	if (pgxp_mem_on == on)
		return 0;
	pgxp_mem_on = on;
	return 1;
}
/* PicaStation emuprof: tag 8 = time inside PGXP display-list lookups,
 * so the emu profile can say whether the 3D recovery costs anything.
 * The frontend defines the tag; this file only stamps it. */
extern volatile int emuprof_tag;

/* exact match by address: no collisions, no ambiguity, no guessing */
static int pgxp_addr_lookup_(u32 addr, u32 val, float *x, float *y, float *z,
                     float *vx, float *vy, float *vz,
                     float *ofx, float *ofy, float *h)
{
	pgxp_shadow_ent *s;
	const pgxp_xf *e;
	u32 w, seq;
	if (!pgxp_shadow || !pgxp_ram_word(addr, &w)) {
		pgxp_addr_miss++;
		return 0;
	}
	s = &pgxp_shadow[w];
	seq = s->seq;
	if (!seq) {
		pgxp_addr_miss++;
		return 0;
	}
	if (s->val != val) {
		/* memory changed behind our back: this word is no longer the
		 * one we recorded, so the transform does not describe it.
		 * This is PGXP's value-validation rule and it is the whole
		 * safety model -- without it a word rewritten by a path we do
		 * not hook inherits someone else's depth. */
		pgxp_addr_stale++;
		return 0;
	}
	if (pgxp_pool_head - seq > PGXP_POOL_N) {
		/* the ring wrapped past it: the transform is provably gone */
		pgxp_addr_evict++;
		return 0;
	}
	e = &pgxp_pool[seq & (PGXP_POOL_N - 1)];
	if (pgxp_epoch - e->epoch > PGXP_SLACK) {
		pgxp_addr_old++;
		return 0;
	}
	{
		/* THE LAST INVARIANT: the linked transform's own screen
		 * position must BE the packet's. val==word only proves the
		 * address holds this value -- it says nothing about whether
		 * the linked transform produced it: a register rebuilt after
		 * MFC2 stores its own value under someone else's seq, and
		 * every earlier check passes. Measured before this gate:
		 * floor pixels carrying w=1919 from a transform that
		 * projected to a different pixel entirely. */
		s32 px = ((s32)((val & 0x7FF) << 21)) >> 21;
		s32 py = ((s32)(((val >> 16) & 0x7FF) << 21)) >> 21;
		float dx = e->x - (float)px, dy = e->y - (float)py;
		if (dx < -1.5f || dx > 1.5f || dy < -1.5f || dy > 1.5f) {
			pgxp_addr_pos++;
			return 0;
		}
	}
	*x = e->x; *y = e->y; *z = e->z;
	*vx = e->vx; *vy = e->vy; *vz = e->vz;
	*ofx = e->ofx; *ofy = e->ofy; *h = e->h;
	pgxp_addr_hit++;
	return 1;
}

int pgxp_addr_lookup(u32 addr, u32 val, float *x, float *y, float *z,
                     float *vx, float *vy, float *vz,
                     float *ofx, float *ofy, float *h)
{
	int prev = emuprof_tag, r;
	emuprof_tag = 8;
	r = pgxp_addr_lookup_(addr, val, x, y, z, vx, vy, vz, ofx, ofy, h);
	emuprof_tag = prev;
	return r;
}

/* notefull.on: route pgxp_note through the pre-slim path below
 * (kept verbatim) instead of the deduplicated one */
int pgxp_note_full;

static void pgxp_note_v1(s64 fx, s64 fy, s32 sx, s32 sy, s32 sz,
               s32 vx, s32 vy, s32 vz, s32 ofx, s32 ofy, s32 h, int slot)
{
	u32 key = pgxp_key((u32)sx, (u32)sy);
	pgxp_ent *e = &pgxp_tab[pgxp_slot(key)];
	float z = (float)sz;
	{
		/* Mirror into the screen FIFO FIRST, built from the ARGUMENTS
		 * rather than from the hash entry. The hash path can bail
		 * early — an ambiguous pixel poisons its slot and returns —
		 * and when it did, the FIFO kept the PREVIOUS vertex's
		 * transform, so the next SWC2 recorded that stale transform
		 * against this vertex's address and the geometry jumped to an
		 * inherited depth. Which register holds what has nothing to do
		 * with whether the pixel key is contested.
		 *
		 * The two ops also fill the FIFO differently, and conflating
		 * them corrupts two slots of three:
		 *   RTPS pushes  — SXY0 <- SXY1 <- SXY2, writes SXY2 (slot 3)
		 *   RTPT writes  — SXY0, SXY1, SXY2 directly (slots 0..2) */
		pgxp_ent f;
		float px = (float)fx / 65536.0f;
		float py = (float)fy / 65536.0f;
		/* SATURATE like the hardware does. limG1/limG2 clamp the
		 * architectural SX2/SY2 to [-1024, 1023] and the depth is
		 * floored at H/2. Keeping the raw projection instead lets a
		 * vertex near or behind the camera carry a position hundreds
		 * of units off-screen while its packed word says "screen
		 * edge" — and no value check can catch that, because the word
		 * IS the clamped one. It validates, then flings the triangle
		 * across the frame. */
		if (px < -1024.0f) px = -1024.0f;
		if (px > 1023.0f) px = 1023.0f;
		if (py < -1024.0f) py = -1024.0f;
		if (py > 1023.0f) py = 1023.0f;
		if (z < (float)h * 0.5f) z = (float)h * 0.5f;
		f.key = key;
		f.epoch = pgxp_epoch;
		f.x = px;
		f.y = py;
		f.z = z;
		f.vx = (float)vx; f.vy = (float)vy; f.vz = (float)vz;
		f.ofx = (float)ofx / 65536.0f;
		f.ofy = (float)ofy / 65536.0f;
		f.h = (float)h;
		f.amb = 0;
		pgxp_truth_note(f.x, f.y, f.z);
		/* Append the transform to the ring, and remember WHICH ring slot
		 * each screen-FIFO register now refers to. The register file
		 * holds an integer word; we hold the float transform that
		 * produced it, and the sequence number is the link between the
		 * two that stays honest after the ring wraps. */
		if (pgxp_pool) {
			u32 seq = pgxp_pool_head++;
			pgxp_xf *xf = &pgxp_pool[seq & (PGXP_POOL_N - 1)];
			xf->epoch = f.epoch;
			xf->x = f.x;   xf->y = f.y;   xf->z = f.z;
			xf->vx = f.vx; xf->vy = f.vy; xf->vz = f.vz;
			xf->ofx = f.ofx; xf->ofy = f.ofy; xf->h = f.h;
			if (slot == 3) {
				pgxp_fifo_seq[0] = pgxp_fifo_seq[1];
				pgxp_fifo_seq[1] = pgxp_fifo_seq[2];
				pgxp_fifo_seq[2] = seq;
			} else if (slot >= 0 && slot < 3) {
				pgxp_fifo_seq[slot] = seq;
			}
		}
	}
	if (e->key == key && e->epoch == pgxp_epoch) {
		/* same pixel, same frame, different depth: one vertex is
		 * behind the other and the key cannot tell them apart.
		 * Poison it rather than pick one. */
		/* RELATIVE tolerance. An absolute 4 units against a depth
		 * range of ~14000 poisoned vertices whose depths differed by
		 * a fraction of a percent -- either value would have been
		 * fine. Only refuse when the two are genuinely at different
		 * distances. */
		float d = e->z - z, lim = z * 0.02f;
		if (d < 0) d = -d;
		if (lim < 2.0f) lim = 2.0f;
		if (d > lim) {
			e->amb = 1;
			pgxp_amb++;
			return;
		}
	} else {
		e->amb = 0;
	}
	e->key = key;
	/* the SATURATED sub-pixel position (f.x/f.y), never the raw
	 * projection: rebuilding from fx/fy here skipped the limG clamp
	 * that every other path applies, so a vertex near the range edge
	 * could pass validation carrying a position hundreds of units
	 * off-screen -- the exact "validates, then flings the triangle"
	 * failure this file already documents. Also four fewer converts. */
	{
		float sx_f = (float)fx / 65536.0f, sy_f = (float)fy / 65536.0f;
		if (sx_f < -1024.0f) sx_f = -1024.0f;
		if (sx_f > 1023.0f) sx_f = 1023.0f;
		if (sy_f < -1024.0f) sy_f = -1024.0f;
		if (sy_f > 1023.0f) sy_f = 1023.0f;
		e->x = sx_f;
		e->y = sy_f;
	}
	e->z = z;                      /* view depth, GTE units */
	e->vx = (float)vx;             /* view space, same units as z */
	e->vy = (float)vy;
	e->vz = (float)vz;
	e->epoch = pgxp_epoch;
	/* PER ENTRY, not global. A game may change the projection between
	 * objects (different FOV for a weapon model, a cutscene camera,
	 * a UI layer), and a single global left every consumer using
	 * whichever values happened to be written last that frame. */
	e->ofx = (float)ofx / 65536.0f;
	e->ofy = (float)ofy / 65536.0f;
	e->h = (float)h;
	pgxp_writes++;
}

/* Deduplicated pgxp_note: identical values to _v1 in every consumer —
 * one set of converts+clamps now serves the truth stream, the pool
 * append AND the hash entry. _v1 computed the clamped position twice
 * and filled a whole pgxp_ent even when the pool (address provenance)
 * was off, which made it ~2x its useful cost per vertex in the
 * shipping config. Semantics notes carried over from _v1: saturate
 * like the hardware (limG1/limG2 to [-1024,1023], depth floored at
 * H/2); RTPS fills FIFO slot 3 (push), RTPT slots 0..2 directly;
 * ambiguity poisoning uses a relative depth tolerance. */
void pgxp_note(s64 fx, s64 fy, s32 sx, s32 sy, s32 sz,
               s32 vx, s32 vy, s32 vz, s32 ofx, s32 ofy, s32 h, int slot)
{
	u32 key;
	pgxp_ent *e;
	float z, px, py;

	if (pgxp_note_full) {
		pgxp_note_v1(fx, fy, sx, sy, sz, vx, vy, vz, ofx, ofy, h,
			     slot);
		return;
	}
	key = pgxp_key((u32)sx, (u32)sy);
	e = &pgxp_tab[pgxp_slot(key)];
	z = (float)sz;
	px = (float)fx / 65536.0f;
	py = (float)fy / 65536.0f;
	if (px < -1024.0f) px = -1024.0f;
	if (px > 1023.0f) px = 1023.0f;
	if (py < -1024.0f) py = -1024.0f;
	if (py > 1023.0f) py = 1023.0f;
	if (z < (float)h * 0.5f) z = (float)h * 0.5f;
	pgxp_truth_note(px, py, z);
	if (pgxp_pool) {
		u32 seq = pgxp_pool_head++;
		pgxp_xf *xf = &pgxp_pool[seq & (PGXP_POOL_N - 1)];
		xf->epoch = pgxp_epoch;
		xf->x = px;   xf->y = py;   xf->z = z;
		xf->vx = (float)vx; xf->vy = (float)vy; xf->vz = (float)vz;
		xf->ofx = (float)ofx / 65536.0f;
		xf->ofy = (float)ofy / 65536.0f;
		xf->h = (float)h;
		if (slot == 3) {
			pgxp_fifo_seq[0] = pgxp_fifo_seq[1];
			pgxp_fifo_seq[1] = pgxp_fifo_seq[2];
			pgxp_fifo_seq[2] = seq;
		} else if (slot >= 0 && slot < 3) {
			pgxp_fifo_seq[slot] = seq;
		}
	}
	if (e->key == key && e->epoch == pgxp_epoch) {
		float d = e->z - z, lim = z * 0.02f;
		if (d < 0) d = -d;
		if (lim < 2.0f) lim = 2.0f;
		if (d > lim) {
			e->amb = 1;
			pgxp_amb++;
			return;
		}
	} else {
		e->amb = 0;
	}
	e->key = key;
	e->x = px;
	e->y = py;
	e->z = z;                      /* view depth, GTE units */
	e->vx = (float)vx;             /* view space, same units as z */
	e->vy = (float)vy;
	e->vz = (float)vz;
	e->epoch = pgxp_epoch;
	e->ofx = (float)ofx / 65536.0f;
	e->ofy = (float)ofy / 65536.0f;
	e->h = (float)h;
	pgxp_writes++;
}

/* ---- capture from the REGISTER FILE, after an asm fast-path op ----
 * With capture on, RTPS/RTPT used to fall back to the C handlers just
 * to keep the sub-pixel projection and per-vertex view vectors. Both
 * were audited diagnostic-only: the shipping stereo consumes only the
 * DEPTH, and orbit re-projects from screen+depth by design (the
 * captured IR vectors are deliberately never used). Everything else
 * capture needs survives the op in the register file — so the dynarec
 * now keeps the hand-scheduled asm handler and emits one call to
 * these afterwards. px/py capture as the integer SXY the packet will
 * carry, which makes the correlation gate exact by construction.
 * asmcap.off restores the old C-handler fallback. */
int pgxp_asmcap_off;

void pgxp_note_rtps_regs(psxCP2Regs *regs)
{
	u32 sxy = regs->CP2D.r[14];              /* SXY2 (just pushed) */
	s32 sx = (s32)(s16)(sxy & 0xffff), sy = (s32)sxy >> 16;
	s32 sz = (s32)(u16)regs->CP2D.r[19];     /* SZ3 */
	pgxp_note((s64)sx << 16, (s64)sy << 16, sx, sy, sz,
	          (s32)regs->CP2D.r[9], (s32)regs->CP2D.r[10], sz,
	          (s32)regs->CP2C.r[24], (s32)regs->CP2C.r[25],
	          (s32)(u16)regs->CP2C.r[26], 3); /* RTPS: FIFO push */
}

void pgxp_note_rtpt_regs(psxCP2Regs *regs)
{
	int v;
	for (v = 0; v < 3; v++) {
		u32 sxy = regs->CP2D.r[12 + v];      /* SXY0..2 */
		s32 sx = (s32)(s16)(sxy & 0xffff), sy = (s32)sxy >> 16;
		s32 sz = (s32)(u16)regs->CP2D.r[17 + v]; /* SZ1..3 */
		/* IR1/IR2 hold only the LAST vertex's view vector; the
		 * earlier ones are gone — pass zero (audited unused) */
		pgxp_note((s64)sx << 16, (s64)sy << 16, sx, sy, sz,
		          v == 2 ? (s32)regs->CP2D.r[9] : 0,
		          v == 2 ? (s32)regs->CP2D.r[10] : 0, sz,
		          (s32)regs->CP2C.r[24], (s32)regs->CP2C.r[25],
		          (s32)(u16)regs->CP2C.r[26], v);
	}
}

/* returns 1 and fills x/y/z when this packed XY was produced by a
 * transform we saw; 0 when it was not (2D art, CPU-built geometry) */
int pgxp_lookup_proj(u32 packed, float *ofx, float *ofy, float *h)
{
	u32 key = packed & 0x07FF07FFu;
	pgxp_ent *e = &pgxp_tab[pgxp_slot(key)];
	if (!e->epoch || e->key != key || e->amb)
		return 0;
	if (pgxp_epoch - e->epoch > PGXP_SLACK)
		return 0;
	*ofx = e->ofx; *ofy = e->ofy; *h = e->h;
	return 1;
}

int pgxp_lookup_v(u32 packed, float *vx, float *vy, float *vz)
{
	u32 key = packed & 0x07FF07FFu;
	pgxp_ent *e = &pgxp_tab[pgxp_slot(key)];
	if (!e->epoch || e->key != key || e->amb)
		return 0;
	if (pgxp_epoch - e->epoch > PGXP_SLACK)
		return 0;
	*vx = e->vx; *vy = e->vy; *vz = e->vz;
	return 1;
}

static int pgxp_lookup_(u32 packed, float *x, float *y, float *z)
{
	/* The 11-bit mask is NOT cosmetic: some games pack extra data into
	 * the top bits of a vertex word (THPS is the documented case), so
	 * only the coordinate field may take part in the key. */
	u32 key = packed & 0x07FF07FFu;
	pgxp_ent *e = &pgxp_tab[pgxp_slot(key)];
	/* WHY a lookup fails matters: "never transformed" means the game
	 * reused geometry we never saw the GTE produce, which no amount of
	 * table tuning can fix. "wrong key" means we evicted it. */
	if (!e->epoch) {
		pgxp_m_empty++;
		return 0;
	}
	if (e->key != key) {
		pgxp_m_key++;
		return 0;
	}
	if (e->amb) {
		pgxp_m_amb++;
		return 0;
	}
	if (pgxp_epoch - e->epoch > PGXP_SLACK) {
		/* transformed too long ago to be this packet's provenance —
		 * a coincidental match on the same pixel, not the same
		 * vertex. Slack rather than an exact frame because the epoch
		 * ticks on VBLANK (~60Hz) while a game may render at 30fps,
		 * and display lists are typically double-buffered: geometry
		 * is transformed a frame or more before it is drawn. One
		 * epoch of slack measured 0% hits for exactly this reason. */
		pgxp_stale++;
		return 0;
	}
	*x = e->x; *y = e->y; *z = e->z;
	return 1;
}

int pgxp_lookup(u32 packed, float *x, float *y, float *z)
{
	int prev = emuprof_tag, r;
	emuprof_tag = 8;
	r = pgxp_lookup_(packed, x, y, z);
	emuprof_tag = prev;
	return r;
}

void gteDispatch(psxCP2Regs *regs, u32 code)
{
	gte_handler *h = gteGetHandler(code);
	if (h)
		h(regs, code);
	else
		log_unhandled("unhandled gte op %08x\n", code);
}

/* decomposed/parametrized versions for the recompiler */

void gteSQR_part_noshift(psxCP2Regs *regs) {
	gteMAC1 = gteIR1 * gteIR1;
	gteMAC2 = gteIR2 * gteIR2;
	gteMAC3 = gteIR3 * gteIR3;
}

void gteSQR_part_shift(psxCP2Regs *regs) {
	gteMAC1 = (gteIR1 * gteIR1) >> 12;
	gteMAC2 = (gteIR2 * gteIR2) >> 12;
	gteMAC3 = (gteIR3 * gteIR3) >> 12;
}

void gteOP_part_noshift(psxCP2Regs *regs) {
	gteMAC1 = (gteR22 * gteIR3) - (gteR33 * gteIR2);
	gteMAC2 = (gteR33 * gteIR1) - (gteR11 * gteIR3);
	gteMAC3 = (gteR11 * gteIR2) - (gteR22 * gteIR1);
}

void gteOP_part_shift(psxCP2Regs *regs) {
	gteMAC1 = ((gteR22 * gteIR3) - (gteR33 * gteIR2)) >> 12;
	gteMAC2 = ((gteR33 * gteIR1) - (gteR11 * gteIR3)) >> 12;
	gteMAC3 = ((gteR11 * gteIR2) - (gteR22 * gteIR1)) >> 12;
}

void gteGPF_part_noshift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = gteIR0 * gteIR1;
	gteMAC2 = gteIR0 * gteIR2;
	gteMAC3 = gteIR0 * gteIR3;
}

void gteGPF_part_shift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = (gteIR0 * gteIR1) >> 12;
	gteMAC2 = (gteIR0 * gteIR2) >> 12;
	gteMAC3 = (gteIR0 * gteIR3) >> 12;
}

void gteGPL_part_noshift(psxCP2Regs *regs) {
	s16 ir0 = gteIR0;
	gteFLAG = 0;

	gteMAC1 += ir0 * gteIR1;
	gteMAC2 += ir0 * gteIR2;
	gteMAC3 += ir0 * gteIR3;
}

#endif // !FLAGLESS

void NM(gteGPL_part_shift)(psxCP2Regs *regs) {
	s16 ir0 = gteIR0;
	u32 flags = 0;
	gteMAC1 = mac123add_s12(1, &flags, gteMAC1, ir0 * gteIR1, 12);
	gteMAC2 = mac123add_s12(2, &flags, gteMAC2, ir0 * gteIR2, 12);
	gteMAC3 = mac123add_s12(3, &flags, gteMAC3, ir0 * gteIR3, 12);
	gteFLAG = getFinalFlag(flags);
}

static inline void runColor2part(psxCP2Regs *regs, int shift, s32 mac1, s32 mac2, s32 mac3)
{
	s32 ir1, ir2, ir3;
	s32 ir0 = gteIR0;
	u32 flags = 0;

	ir1 = limB1(&flags, mac123sub_s12(1, &flags, gteRFC, mac1, shift), 0);
	ir2 = limB2(&flags, mac123sub_s12(2, &flags, gteGFC, mac2, shift), 0);
	ir3 = limB3(&flags, mac123sub_s12(3, &flags, gteBFC, mac3, shift), 0);
	gteMAC1 = (ir0 * ir1 + (s64)mac1) >> shift;
	gteMAC2 = (ir0 * ir2 + (s64)mac2) >> shift;
	gteMAC3 = (ir0 * ir3 + (s64)mac3) >> shift;
	gteFLAG = flags;
}

void NM(gteDPCS_part_noshift)(psxCP2Regs *regs) {
	runColor2part(regs, 0, gteR << 16, gteG << 16, gteB << 16);
}

void NM(gteDPCS_part_shift)(psxCP2Regs *regs) {
	runColor2part(regs, 12, gteR << 16, gteG << 16, gteB << 16);
}

void NM(gteINTPL_part_noshift)(psxCP2Regs *regs) {
	runColor2part(regs, 0, gteIR1 << 12, gteIR2 << 12, gteIR3 << 12);
}

void NM(gteINTPL_part_shift)(psxCP2Regs *regs) {
	runColor2part(regs, 12, gteIR1 << 12, gteIR2 << 12, gteIR3 << 12);
}

void NM(gteMACtoRGB)(psxCP2Regs *regs) {
	u32 flags = gteFLAG;
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(&flags, gteMAC1 >> 4);
	gteG2 = limC2(&flags, gteMAC2 >> 4);
	gteB2 = limC3(&flags, gteMAC3 >> 4);
	gteFLAG = getFinalFlag(flags);
}
