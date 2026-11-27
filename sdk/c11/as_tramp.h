// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* as_tramp.h - x86-64 call trampoline for registered C functions.
 * Reuses sdk/asm/callx64_nasm.asm (callx64_sysv, SysV AMD64).
 * The trampoline loads up to 6 GP args and 4 FP args from arrays and
 * returns the callee's integer result in rax (float/double in xmm0).
 * One asm symbol (callx64_sysv) is reached through three C prototypes
 * (integer / float / double return) using GCC asm labels, so each call
 * goes through a compatible function-pointer type and the caller reads
 * rax for integer results and xmm0 for float/double results. This is
 * the documented pattern for this trampoline.
 * The target is an asFUNCTION_t (a function pointer), so no
 * function-pointer-to-object-pointer conversion is needed.
 * On non-x86-64 targets only 0-argument void functions are supported
 * (called directly); anything else returns asNOT_SUPPORTED.
 */

#ifndef AS_C11_TRAMP_H
#define AS_C11_TRAMP_H

#include <stdint.h>
#include "angelscript_c.h"

#if defined(__x86_64__)
/* One asm symbol, three C prototypes (GCC asm labels). */
extern uint64_t callx64_sysv(asFUNCTION_t target, const uint64_t *gp_args,
                             const double *fp_args, int n_gp, int n_fp);
extern double callx64_sysv_dbl(asFUNCTION_t target, const uint64_t *gp_args,
                               const double *fp_args, int n_gp, int n_fp)
    __asm__("callx64_sysv");
extern float callx64_sysv_flt(asFUNCTION_t target, const uint64_t *gp_args,
                              const double *fp_args, int n_gp, int n_fp)
    __asm__("callx64_sysv");

/* Integer/pointer/void return: result in rax. */
static inline uint64_t as_tramp_call_int(asFUNCTION_t target, const uint64_t *gp,
                                         const double *fp, int n_gp, int n_fp)
{
    return callx64_sysv(target, gp, fp, n_gp, n_fp);
}

/* Double return: result in xmm0. */
static inline double as_tramp_call_double(asFUNCTION_t target, const uint64_t *gp,
                                          const double *fp, int n_gp, int n_fp)
{
    return callx64_sysv_dbl(target, gp, fp, n_gp, n_fp);
}

/* Float return: result in the low 32 bits of xmm0. */
static inline float as_tramp_call_float(asFUNCTION_t target, const uint64_t *gp,
                                        const double *fp, int n_gp, int n_fp)
{
    return callx64_sysv_flt(target, gp, fp, n_gp, n_fp);
}

#else /* not x86-64 */
static inline uint64_t as_tramp_call_int(asFUNCTION_t target, const uint64_t *gp,
                                         const double *fp, int n_gp, int n_fp)
{
    (void)gp; (void)fp; (void)n_gp; (void)n_fp;
    /* Direct call, 0-argument void functions only. */
    ((void (*)(void))target)();
    return 0;
}
static inline double as_tramp_call_double(asFUNCTION_t target, const uint64_t *gp,
                                          const double *fp, int n_gp, int n_fp)
{
    (void)target; (void)gp; (void)fp; (void)n_gp; (void)n_fp;
    return 0.0;
}
static inline float as_tramp_call_float(asFUNCTION_t target, const uint64_t *gp,
                                        const double *fp, int n_gp, int n_fp)
{
    (void)target; (void)gp; (void)fp; (void)n_gp; (void)n_fp;
    return 0.0f;
}
#endif

#endif /* AS_C11_TRAMP_H */
