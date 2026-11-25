(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
// FILE: proofs/abi_contract.sats
(* ATS2 linear/dependent proofs for VM ABI v2 *)
(*
 * Target: VM ABI v2 (VM_ABI_VERSION 0x00020000u), include/vm_abi.h.
 * Sizes (gcc 13.3.0, x86-64, _Alignas(16) on every struct):
 *   vm_region_t 32, vm_module_hdr_t 64, vm_exec_ctx_t 176, vm_trace_rec_t 48;
 *   alignment 16 on all four.
 * The v1 claim of a 128-byte vm_exec_ctx_t is mathematically impossible:
 * the 16 GPRs alone fill 128 bytes, and rip + rflags add 16 more (144)
 * before state, priv_ring and caps are counted. v2 declares the real 176.
 * Offsets (pinned in vm_abi.h): hdr magic 0, version 4, abi_hash 8,
 * proof_hash 16, caps 24, entry_off 32, code_size 40, n_regions 48,
 * _pad 52, _reserved 56; ctx rax 0, rbx 8, rcx 16, rdx 24, rsi 32,
 * rdi 40, rbp 48, rsp 56, r8 64, r9 72, r10 80, r11 88, r12 96, r13 104,
 * r14 112, r15 120, rip 128, rflags 136, state 144, priv_ring 148,
 * caps 152, _pad 160, _reserved 168; trace tsc 0, rip 8, op 16,
 * ctx_id 20, data0 24, data1 32, _reserved 40; region base 0, size 8,
 * used 16, flags 24, _pad 28.
 *
 * Static vs dynamic capability masks: the C type vm_cap_mask_t
 * (uint64_t) is a dynamic runtime type and cannot appear in a static
 * proposition. Capability masks are therefore modelled statically as
 * [capmask] (a static int, 0..63 -- VM_CAP_ALL is 0x3F, bits 0..5) and
 * the subset relation is [cap_admitted], stated per defined bit with
 * div/mod bit tests: bit b of x is set iff (x / b) % 2 == 1. ATS2
 * static terms have no bitwise [land], so the original
 * [(m land r) == r] could not be written; the per-bit form states the
 * same relation (every bit set in [r] is set in [m]) in operators the
 * static language provides, and the constraint solver discharges true
 * instances and rejects false ones (e.g. mask 2 does not admit 1).
 *)
staload "prelude/basics_pre.sats"
staload "prelude/SATS/integer.sats"
staload "prelude/SATS/pointer.sats"

abstype vm_cap_mask_t = $extype"uint64_t"
abstype vm_region_t = $extype"struct vm_region"
abstype vm_module_hdr_t = $extype"struct vm_module_hdr"
abstype vm_exec_ctx_t = $extype"struct vm_exec_ctx"
abstype vm_trace_rec_t = $extype"struct vm_trace_rec"
abstype vm_err_t = $extype"int32_t"

(* Static capability masks: static ints, not the dynamic C type above. *)
sortdef capmask = int

(* Capability bits, mirroring VM_CAP_* in vm_abi.h. *)
stadef CAP_RING0 = 1
stadef CAP_MEM = 2
stadef CAP_C11 = 4
stadef CAP_JIT = 8
stadef CAP_TRACE = 16
stadef CAP_REPL = 32

(* Bit test: bit b (a power of two) of x is set iff (x / b) % 2 == 1. *)
stadef cap_bit (x: int, b: int): int = (x / b) % 2

(* [cap_admitted(m, r)]: every capability bit set in r is set in m.
 * Both masks must lie in the defined capability space 0..63. *)
stadef cap_admitted (m: int, r: int): bool =
  (0 <= m && m < 64 && 0 <= r && r < 64) &&
  ((cap_bit(r, CAP_RING0) == 0) || (cap_bit(m, CAP_RING0) == 1)) &&
  ((cap_bit(r, CAP_MEM) == 0) || (cap_bit(m, CAP_MEM) == 1)) &&
  ((cap_bit(r, CAP_C11) == 0) || (cap_bit(m, CAP_C11) == 1)) &&
  ((cap_bit(r, CAP_JIT) == 0) || (cap_bit(m, CAP_JIT) == 1)) &&
  ((cap_bit(r, CAP_TRACE) == 0) || (cap_bit(m, CAP_TRACE) == 1)) &&
  ((cap_bit(r, CAP_REPL) == 0) || (cap_bit(m, CAP_REPL) == 1))

absprop ALIGN16 (a: t@ype)
absprop NONNULL (p: addr)
absprop REGION_BOUNDED (base: addr, len: int)
absprop CAP_ADMITTED (mask: capmask, required: capmask)

praxi align16_lemma {a:t@ype} (): ALIGN16(a)
praxi nonnull_lemma {p:addr | p > null} (): NONNULL(p)
praxi region_bounded_lemma {b:addr; n:nat | n > 0} (): REGION_BOUNDED(b, n)
praxi cap_admit_lemma {m:capmask; r:capmask | cap_admitted(m, r)} (): CAP_ADMITTED(m, r)

fun sizeof_vm_region (): size_t = "mac#sizeof(struct vm_region)"
fun sizeof_vm_module_hdr (): size_t = "mac#sizeof(struct vm_module_hdr)"
fun sizeof_vm_exec_ctx (): size_t = "mac#sizeof(struct vm_exec_ctx)"
fun sizeof_vm_trace_rec (): size_t = "mac#sizeof(struct vm_trace_rec)"

prfun prove_align16_region (): ALIGN16(vm_region_t)
prfun prove_align16_hdr (): ALIGN16(vm_module_hdr_t)
prfun prove_align16_ctx (): ALIGN16(vm_exec_ctx_t)
prfun prove_align16_trace (): ALIGN16(vm_trace_rec_t)

prfun prove_size_region (): [n:int | n == 32] int(n)
prfun prove_size_hdr (): [n:int | n == 64] int(n)
prfun prove_size_ctx (): [n:int | n == 176] int(n)
prfun prove_size_trace (): [n:int | n == 48] int(n)
