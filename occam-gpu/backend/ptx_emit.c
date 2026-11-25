// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "backend.h"
#include <stdio.h>
#include <string.h>

int backend_emit_ptx(const gpu_module_t *m, const backend_opts_t *opt,
                     char *buf, size_t bufsz, size_t *out_len)
{
    if (!m || !buf || !out_len) return -1;
    size_t pos = 0;
    int n;

    n = snprintf(buf + pos, bufsz - pos,
                 ".version 7.0\n.target sm_%u\n.address_size 64\n\n",
                 opt ? opt->sm_version : 80);
    if (n < 0 || (size_t)n >= bufsz - pos) return -1;
    pos += n;

    for (uint32_t k = 0; k < m->n_kernels; k++) {
        const gpu_kernel_t *ker = &m->kernels[k];
        n = snprintf(buf + pos, bufsz - pos,
                     ".visible .entry k_%u(.param .u64 param0)\n{\n"
                     " .reg .u32 %%r<16>;\n"
                     " .reg .u64 %%rd<8>;\n"
                     " // workgroup %u x %u x %u\n",
                     ker->id, ker->wg_x, ker->wg_y, ker->wg_z);
        if (n < 0 || (size_t)n >= bufsz - pos) return -1;
        pos += n;

        for (uint32_t b = 0; b < ker->n_blocks; b++) {
            const gpu_block_t *blk = &m->blocks[ker->block_start + b];
            n = snprintf(buf + pos, bufsz - pos, "BB_%u:\n", blk->id);
            if (n < 0 || (size_t)n >= bufsz - pos) return -1;
            pos += n;
            for (uint32_t i = 0; i < blk->n_instrs; i++) {
                const gpu_instr_t *in = &m->instrs[blk->instr_start + i];
                switch (in->op) {
                case GOP_ADD:
                    n = snprintf(buf + pos, bufsz - pos,
                                 " add.s32 %%r%u, %%r%u, %%r%u;\n",
                                 in->dst, in->src0, in->src1);
                    break;
                case GOP_MUL:
                    n = snprintf(buf + pos, bufsz - pos,
                                 " mul.lo.s32 %%r%u, %%r%u, %%r%u;\n",
                                 in->dst, in->src0, in->src1);
                    break;
                case GOP_WORKITEM_ID:
                    n = snprintf(buf + pos, bufsz - pos,
                                 " mov.u32 %%r%u, %%tid.x;\n", in->dst);
                    break;
                case GOP_BARRIER:
                    n = snprintf(buf + pos, bufsz - pos,
                                 " bar.sync 0;\n");
                    break;
                case GOP_CHAN_SEND:
                    n = snprintf(buf + pos, bufsz - pos,
                                 " // chan_send %u\n", in->src0);
                    break;
                case GOP_CHAN_RECV:
                    n = snprintf(buf + pos, bufsz - pos,
                                 " // chan_recv %u -> %%r%u\n", in->src0, in->dst);
                    break;
                default:
                    n = snprintf(buf + pos, bufsz - pos, " // op %u\n", in->op);
                    break;
                }
                if (n < 0 || (size_t)n >= bufsz - pos) return -1;
                pos += n;
            }
        }
        n = snprintf(buf + pos, bufsz - pos, " ret;\n}\n\n");
        if (n < 0 || (size_t)n >= bufsz - pos) return -1;
        pos += n;
    }
    *out_len = pos;
    return 0;
}
