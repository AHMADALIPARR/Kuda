// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// mamba_bind.h — Mamba Binding Layer for SM89 sovereign kernels
// Author: Ahmad Ali Parr · Trust: Bel Esprit D'Accord Irrevocable Trust

#ifndef MAMBA_BIND_H
#define MAMBA_BIND_H

#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cstdint>

struct MambaAttentionGEMMPipeline;
using PipelineHandle = MambaAttentionGEMMPipeline*;

struct MambaParams {
    half* A_log;     // [d_state, d_inner]
    half* B;         // [d_state, d_inner]
    half* C;         // [d_state, d_inner]
    half* D;         // [d_inner]
    half* dt_proj;   // [d_inner]
    int   d_inner, d_state, d_conv;
    float dt_min, dt_max, dt_init_floor;
};

struct KVPageManager {
    uint64_t* page_table;
    half*     kv_pages;
    int*      page_refcount;
    int*      free_list;
    int       free_head, free_tail, total_pages, page_size, head_dim;
};

struct GGUFQuantization {
    enum Type { Q4_0, Q4_K, Q5_0, Q8_0 } type;
    const void* packed_data;
    const half* scales;
    const half* mins;
    int block_size;
    int num_blocks;
};

struct MambaPerfMetrics {
    float attn_ms, gemm_ms, mamba_ms, copy_ms, dequant_ms, total_ms;
    float attn_tflops, gemm_tflops;
    int64_t bytes_moved;
};

#ifdef __cplusplus
extern "C" {
#endif

PipelineHandle mamba_create_pipeline(int batch, int heads, int seq, int dim, int pages);
void           mamba_destroy_pipeline(PipelineHandle h);
void           mamba_execute(PipelineHandle h);
void           mamba_execute_async(PipelineHandle h, int steps);

void mamba_launch_flash_attention(
    const half* q, const half* k, const half* v, half* o,
    const uint64_t* page_table,
    int batch, int heads, int seq, int dim,
    int page_size, int num_pages, cudaStream_t stream
);

void mamba_launch_dequant_gguf(
    const GGUFQuantization* quant,
    half* output,
    cudaStream_t stream
);

MambaPerfMetrics mamba_benchmark(PipelineHandle h, int warmup, int iterations);

#ifdef __cplusplus
}
#endif

#endif // MAMBA_BIND_H
