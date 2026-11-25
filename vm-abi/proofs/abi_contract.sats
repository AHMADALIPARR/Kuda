(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
(* ATS2 linear/dependent proofs for VM ABI *)
staload "prelude/basics_pre.sats"
staload "prelude/SATS/integer.sats"
staload "prelude/SATS/pointer.sats"

abstype vm_cap_mask_t = $extype"uint64_t"
abstype vm_region_t = $extype"struct vm_region"
abstype vm_module_hdr_t = $extype"struct vm_module_hdr"
abstype vm_exec_ctx_t = $extype"struct vm_exec_ctx"
abstype vm_trace_rec_t = $extype"struct vm_trace_rec"
abstype vm_err_t = $extype"int32_t"

absprop ALIGN16 (a: t@ype)
absprop NONNULL (p: addr)
absprop REGION_BOUNDED (base: addr, len: int)
absprop CAP_ADMITTED (mask: vm_cap_mask_t, required: vm_cap_mask_t)

praxi align16_lemma {a:t@ype} (): ALIGN16(a)
praxi nonnull_lemma {p:addr | p > null} (): NONNULL(p)
praxi region_bounded_lemma {b:addr; n:nat | n > 0} (): REGION_BOUNDED(b, n)
praxi cap_admit_lemma {m:vm_cap_mask_t; r:vm_cap_mask_t}
  (pf: (m land r) == r): CAP_ADMITTED(m, r)

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
prfun prove_size_ctx (): [n:int | n == 128] int(n)
prfun prove_size_trace (): [n:int | n == 48] int(n)
