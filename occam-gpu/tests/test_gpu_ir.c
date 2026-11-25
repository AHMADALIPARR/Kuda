// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "../include/occam.h"
#include "../include/gpu_ir.h"
#include "../include/backend.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static const char *sample_occam =
    "PROC main()\n"
    " PAR\n"
    " c ? x\n"
    " c ! y\n"
    " SEQ\n"
    " SKIP\n"
    ":\n";

int main(void)
{
    occam_module_t *oc = occam_parse(sample_occam, strlen(sample_occam));
    assert(oc != NULL);
    assert(occam_validate_csp(oc) == 0);

    gpu_module_t *m = gpu_ir_create();
    assert(m != NULL);
    assert(gpu_ir_from_occam(m, oc) == 0);
    assert(gpu_ir_verify(m) == 0);
    assert(m->n_kernels >= 1 || m->n_blocks >= 1);

    char ptxbuf[8192];
    size_t ptxlen = 0;
    backend_opts_t opt = { .kind = BACKEND_PTX, .sm_version = 80, .opt_level = 2 };
    assert(backend_emit_ptx(m, &opt, ptxbuf, sizeof(ptxbuf), &ptxlen) == 0);
    assert(ptxlen > 0);
    assert(strstr(ptxbuf, ".entry") != NULL);

    uint8_t amdbuf[4096];
    size_t amdlen = 0;
    opt.kind = BACKEND_AMDGPU;
    opt.gfx_level = 11;
    assert(backend_emit_amdgpu(m, &opt, amdbuf, sizeof(amdbuf), &amdlen) == 0);
    assert(amdlen >= sizeof(uint32_t) * 4);

    uint32_t spirvbuf[256];
    size_t spirvlen = 0;
    opt.kind = BACKEND_SPIRV;
    assert(backend_emit_spirv(m, &opt, spirvbuf, 256, &spirvlen) == 0);
    assert(spirvbuf[0] == 0x07230203u);

    gpu_ir_destroy(m);
    occam_free(oc);
    printf("Occam->GPU IR->PTX/AMDGPU/SPIR-V tests OK\n");
    return 0;
}
