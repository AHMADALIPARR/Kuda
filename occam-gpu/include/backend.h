// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef BACKEND_H
#define BACKEND_H

#include "gpu_ir.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BACKEND_PTX = 0,
    BACKEND_SASS,
    BACKEND_AMDGPU,
    BACKEND_SPIRV
} backend_kind_t;

typedef struct backend_opts {
    backend_kind_t kind;
    uint32_t sm_version; /* e.g. 80 for Ampere */
    uint32_t gfx_level; /* e.g. 11 for RDNA3 */
    int opt_level;
    int emit_sass;
} backend_opts_t;

int backend_emit_ptx(const gpu_module_t *m, const backend_opts_t *opt,
                     char *buf, size_t bufsz, size_t *out_len);
int backend_emit_amdgpu(const gpu_module_t *m, const backend_opts_t *opt,
                        uint8_t *buf, size_t bufsz, size_t *out_len);
int backend_emit_spirv(const gpu_module_t *m, const backend_opts_t *opt,
                       uint32_t *buf, size_t bufsz, size_t *out_len);
int backend_select_and_emit(const gpu_module_t *m, const backend_opts_t *opt,
                            void *buf, size_t bufsz, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif
