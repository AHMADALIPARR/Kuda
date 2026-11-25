// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// FILE: src/c11_substrate.c
#include "vm_abi.h"
#include <string.h>

#define FNV_OFFSET 14695981039346656037ull
#define FNV_PRIME 1099511628211ull

static uint64_t fnv1a_64(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint64_t h = FNV_OFFSET;
    size_t i;
    for (i = 0; i < len; i++) {
        h ^= (uint64_t)p[i];
        h *= FNV_PRIME;
    }
    return h;
}

/* Minimal SHA-256 (public domain style, fixed) */
typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t data[64];
    uint32_t datalen;
} sha256_ctx;

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR(x,n) (((x) >> (n)) | ((x) << (32-(n))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define EP1(x) (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define SIG0(x) (ROTR(x,7) ^ ROTR(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10))

static void sha256_transform(sha256_ctx *ctx, const uint8_t data[])
{
    uint32_t a,b,c,d,e,f,g,h,i,j,t1,t2,m[64];
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j+1] << 16) |
               ((uint32_t)data[j+2] << 8) | ((uint32_t)data[j+3]);
    for (; i < 64; ++i)
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(sha256_ctx *ctx)
{
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(sha256_ctx *ctx, const uint8_t data[], size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(sha256_ctx *ctx, uint8_t hash[])
{
    uint32_t i = ctx->datalen;
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = (uint8_t)(ctx->bitlen);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 8);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 16);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 24);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 32);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 40);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 48);
    ctx->data[56] = (uint8_t)(ctx->bitlen >> 56);
    sha256_transform(ctx, ctx->data);
    for (i = 0; i < 4; ++i) {
        hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0xff;
        hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0xff;
        hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0xff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0xff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0xff;
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0xff;
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0xff;
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0xff;
    }
}

uint64_t vm_abi_hash(const void *hdr, size_t len)
{
    return fnv1a_64(hdr, len);
}

void vm_sha256(const void *data, size_t len, uint8_t out[32])
{
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)data, len);
    sha256_final(&ctx, out);
}

/* Deterministic bump allocator */
typedef struct {
    uint8_t *base;
    uint64_t size;
    uint64_t used;
    uint32_t sealed;
} bump_region_t;

static bump_region_t g_regions[VM_MAX_REGIONS];
static uint32_t g_n_regions;

vm_err_t vm_mem_init(void *arena, uint64_t size)
{
    if (!arena || size < VM_PAGE_SIZE || ((uintptr_t)arena & (VM_ALIGN16 - 1)))
        return VM_ERR_INVAL;
    memset(g_regions, 0, sizeof(g_regions));
    g_n_regions = 1;
    g_regions[0].base = (uint8_t *)arena;
    g_regions[0].size = size;
    g_regions[0].used = 0;
    g_regions[0].sealed = 0;
    return VM_OK;
}

void *vm_mem_alloc(uint32_t reg, uint64_t size, uint64_t align)
{
    bump_region_t *r;
    uint64_t off, pad;
    if (reg >= g_n_regions || size == 0 || align == 0 || (align & (align - 1)))
        return NULL;
    r = &g_regions[reg];
    if (r->sealed)
        return NULL;
    off = r->used;
    pad = (align - (off & (align - 1))) & (align - 1);
    if (off + pad + size > r->size)
        return NULL;
    r->used = off + pad + size;
    return r->base + off + pad;
}

vm_err_t vm_mem_seal(uint32_t reg)
{
    if (reg >= g_n_regions)
        return VM_ERR_INVAL;
    g_regions[reg].sealed = 1;
    return VM_OK;
}

vm_err_t vm_mem_free_region(uint32_t reg)
{
    if (reg >= g_n_regions)
        return VM_ERR_INVAL;
    g_regions[reg].used = 0;
    g_regions[reg].sealed = 0;
    return VM_OK;
}

/* ABI verification.
 * The abi_hash input is the first sizeof(*hdr) - sizeof(proof_hash)
 * bytes (v2 bytes 0..55: magic, version, caps, entry_off, code_size,
 * n_regions, _pad) with BOTH hash fields zeroed. Hashing the stored
 * header as-is cannot work: abi_hash is inside its own input range
 * (offset 8), so the value being checked would change the hash, and
 * proof_hash (offset 16) is written after abi_hash is computed (the
 * test computes abi_hash first, then fills proof_hash from the code
 * digest), so a recomputation over the live bytes could never agree
 * with the stored value. Zeroing both fields in a local copy makes
 * the stored hash and the recomputed hash the same function of the
 * same non-hash fields; the check still fails (-5) on any change to
 * magic, version, caps, entry_off, code_size, n_regions or _pad.
 * The trailing _reserved (56..63) stays outside the input range.
 */
vm_err_t vm_verify_module(const vm_module_hdr_t *hdr, const void *code, size_t code_len)
{
    uint64_t h;
    uint8_t digest[32];
    vm_module_hdr_t hdr_zeroed;
    if (!hdr)
        return VM_ERR_INVAL;
    if (hdr->magic != VM_MODULE_MAGIC)
        return VM_ERR_MAGIC;
    if (hdr->version != VM_ABI_VERSION)
        return VM_ERR_VERSION;
    hdr_zeroed = *hdr;
    hdr_zeroed.abi_hash = 0;
    hdr_zeroed.proof_hash = 0;
    h = fnv1a_64(&hdr_zeroed, sizeof(hdr_zeroed) - sizeof(hdr_zeroed.proof_hash));
    if (h != hdr->abi_hash)
        return VM_ERR_HASH;
    if (code && code_len) {
        vm_sha256(code, code_len, digest);
        /* proof_hash is first 8 bytes of SHA-256 for compactness */
        if (memcmp(digest, &hdr->proof_hash, 8) != 0)
            return VM_ERR_HASH;
    }
    if ((hdr->caps & VM_CAP_ALL) != hdr->caps)
        return VM_ERR_CAP;
    return VM_OK;
}

/* SysV AMD64 trampoline helpers (callable from HolyC) */
uint64_t vm_call6(void *fn, uint64_t a0, uint64_t a1, uint64_t a2,
                  uint64_t a3, uint64_t a4, uint64_t a5)
{
    typedef uint64_t (*fn6_t)(uint64_t, uint64_t, uint64_t,
                              uint64_t, uint64_t, uint64_t);
    return ((fn6_t)fn)(a0, a1, a2, a3, a4, a5);
}
