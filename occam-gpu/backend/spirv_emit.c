// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "backend.h"
#include <string.h>

/* Minimal SPIR-V emitter: header + simple entry point skeleton */

int backend_emit_spirv(const gpu_module_t *m, const backend_opts_t *opt,
                       uint32_t *buf, size_t max_words, size_t *out_len)
{
    (void)opt;
    if (!m || !buf || !out_len || max_words < 32) return -1;

    size_t w = 0;
    /* SPIR-V magic + version + generator + bound + schema */
    buf[w++] = 0x07230203u;
    buf[w++] = 0x00010000u; /* 1.0 */
    buf[w++] = 0x00000000u; /* generator */
    buf[w++] = 64u; /* bound */
    buf[w++] = 0u; /* schema */

    /* OpCapability Shader */
    buf[w++] = (2u << 16) | 17u;
    buf[w++] = 1u;

    /* OpMemoryModel Logical GLSL450 */
    buf[w++] = (3u << 16) | 14u;
    buf[w++] = 0u;
    buf[w++] = 1u;

    /* OpEntryPoint GLCompute %1 "k0" */
    buf[w++] = (4u << 16) | 15u;
    buf[w++] = 5u; /* GLCompute */
    buf[w++] = 1u;
    buf[w++] = 0x306B0000u; /* "k0\0" little-endian-ish */

    /* OpExecutionMode %1 LocalSize */
    if (m->n_kernels > 0) {
        buf[w++] = (6u << 16) | 16u;
        buf[w++] = 1u;
        buf[w++] = 17u; /* LocalSize */
        buf[w++] = m->kernels[0].wg_x;
        buf[w++] = m->kernels[0].wg_y;
        buf[w++] = m->kernels[0].wg_z;
    }

    /* OpFunction etc. omitted for skeleton */
    buf[w++] = (5u << 16) | 36u; /* OpFunction */
    buf[w++] = 2u; /* void type id */
    buf[w++] = 1u; /* result id */
    buf[w++] = 0u; /* None */
    buf[w++] = 3u; /* function type */

    buf[w++] = (2u << 16) | 56u; /* OpReturn */
    buf[w++] = (1u << 16) | 56u; /* placeholder */

    buf[w++] = (1u << 16) | 38u; /* OpFunctionEnd */

    *out_len = w * 4;
    return 0;
}
