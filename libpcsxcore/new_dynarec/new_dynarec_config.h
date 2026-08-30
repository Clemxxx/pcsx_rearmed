
// 3DS (ARM11/MPCore): the A8 workaround only costs here — alignment
// nops, a forced r7 move on every jr, inverted backward branches.
#if defined(__arm__) && !defined(ARM11)
#define CORTEX_A8_BRANCH_PREDICTION_HACK 1
#endif

#define USE_MINI_HT 1
//#define REG_PREFETCH 1

// options:
//#define NO_WRITE_EXEC 1
//#define BASE_ADDR_DYNAMIC 1
//#define TC_WRITE_OFFSET 1
//#define NDRC_CACHE_FLUSH_ALL 1

#if defined(__MACH__) || defined(HAVE_LIBNX)
#define NO_WRITE_EXEC 1
#endif
#if defined(VITA) || defined(HAVE_LIBNX)
#define BASE_ADDR_DYNAMIC 1
#endif
#if defined(HAVE_LIBNX)
#define TC_WRITE_OFFSET 1
#endif
// 3DS: was NDRC_CACHE_FLUSH_ALL (any patch = whole-TC dirty = entire
// L1I+BTB wipe on the emu core at its next sync). With the ranged
// kernel flush (ctr_clear_cache_range) + the raised range cutoff in
// new_dyna_clear_cache + the dirty-span list consumed by
// clear_local_cache, the per-page path is strictly cheaper on N3DS.
