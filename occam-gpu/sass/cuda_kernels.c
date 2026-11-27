// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// cuda_kernels.c — Host launch wrappers for SM89 PTX kernels
// SnapKitty Sovereign Kernel — Hopper Architecture
// Compile: nvcc -arch=sm_89 -ptx cuda_kernels.c -o kernels.ptx
// Author: Ahmad Ali Parr · Trust: Bel Esprit D'Accord Irrevocable Trust

#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" void flash_attention_paged(
    const half* q, const half* k, const half* v, half* o,
    const uint64_t* page_table,
    int batch_size, int num_heads, int seq_len, int head_dim,
    int page_size, int num_pages
);

extern "C" void dequant_gguf_q4k(
    const void* packed_ptr,
    const half* scales_ptr,
    const half* mins_ptr,
    half* output_ptr,
    int num_blocks
);

extern "C" void dequant_gguf_q4k_batched(
    const void* packed_ptr,
    const half* scales_ptr,
    const half* mins_ptr,
    half* output_ptr,
    int batch_size, int seq_len, int feat_len
);

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

void launch_flash_attention_paged(
    const half* q, const half* k, const half* v, half* o,
    const uint64_t* page_table,
    int batch_size, int num_heads, int seq_len, int head_dim,
    int page_size, int num_pages
) {
    dim3 grid(batch_size, num_heads, 1);
    dim3 block(256, 1, 1);
    size_t shared_mem = 64 * 1024;
    void* args[] = {
        &q, &k, &v, &o, &page_table,
        &batch_size, &num_heads, &seq_len, &head_dim,
        &page_size, &num_pages
    };
    CUDA_CHECK(cudaLaunchKernel((void*)flash_attention_paged, grid, block, args, shared_mem, 0));
    CUDA_CHECK(cudaDeviceSynchronize());
}

void launch_dequant_gguf_q4k(
    const void* packed_ptr,
    const half* scales_ptr,
    const half* mins_ptr,
    half* output_ptr,
    int num_blocks
) {
    dim3 block(256);
    dim3 grid((num_blocks + 255) / 256);
    void* args[] = { &packed_ptr, &scales_ptr, &mins_ptr, &output_ptr, &num_blocks };
    CUDA_CHECK(cudaLaunchKernel((void*)dequant_gguf_q4k, grid, block, args, 0, 0));
    CUDA_CHECK(cudaDeviceSynchronize());
}

void launch_dequant_gguf_q4k_batched(
    const void* packed_ptr,
    const half* scales_ptr,
    const half* mins_ptr,
    half* output_ptr,
    int batch_size, int seq_len, int feat_len
) {
    int feat_blocks  = (feat_len + 31) / 32;
    int total_blocks = batch_size * seq_len * feat_blocks;
    launch_dequant_gguf_q4k(packed_ptr, scales_ptr, mins_ptr, output_ptr, total_blocks);
}
