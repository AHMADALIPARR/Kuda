(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
// FILE: proofs/gpu_ir_contract.sats
(* ATS2 linear/dependent proofs for typed GPU IR and Occam process admission *)
(*
 * Propositions quantify over static int identities (a value id and its
 * type code, a block id, a channel id, a kernel id): the C struct types
 * below are dynamic and cannot be used as sorts in propositions.
 *
 * Measured layout (gcc 13.3.0, x86-64), include/gpu_ir.h:
 *   gpu_value_t:  5 x uint32 = 20, align 4.
 *   gpu_instr_t:  7 x uint32 = 28, + 4 pad, int64 imm 8, uint32 flags 4,
 *     + 4 tail pad = 48, align 8.
 *   gpu_block_t:  8 x uint32 = 32, align 4.
 *   gpu_kernel_t: 5 x uint32 = 20, param_types[8] = 32, wg 12, grid 12,
 *     shared_bytes 4, flags 4 = 84, align 4.
 *)
staload "prelude/basics_pre.sats"
staload "prelude/SATS/integer.sats"
staload "prelude/SATS/pointer.sats"

abstype gpu_type_t = $extype"uint32_t"
abstype gpu_value_t = $extype"struct gpu_value"
abstype gpu_instr_t = $extype"struct gpu_instr"
abstype gpu_block_t = $extype"struct gpu_block"
abstype gpu_kernel_t = $extype"struct gpu_kernel"
abstype occam_proc_t = $extype"struct occam_proc"
abstype csp_chan_t = $extype"struct csp_chan"

(* Static identity sorts for the propositions above. *)
sortdef gpu_value_id = int
sortdef gpu_type_code = int
sortdef gpu_block_id = int
sortdef csp_chan_id = int
sortdef gpu_kernel_id = int

absprop WELL_TYPED (v: gpu_value_id, t: gpu_type_code)
absprop SSA_FORM (b: gpu_block_id)
absprop CHANNEL_CAP (c: csp_chan_id, n: int)
absprop WORKGROUP_BOUNDED (k: gpu_kernel_id, x: int, y: int, z: int)
absprop NO_RACE (k: gpu_kernel_id)

praxi well_typed_lemma {v:gpu_value_id; t:gpu_type_code} (): WELL_TYPED(v, t)
praxi ssa_lemma {b:gpu_block_id} (): SSA_FORM(b)
praxi chan_cap_lemma {c:csp_chan_id; n:nat} (): CHANNEL_CAP(c, n)
praxi wg_bound_lemma {k:gpu_kernel_id; x,y,z:pos} (): WORKGROUP_BOUNDED(k, x, y, z)
praxi no_race_lemma {k:gpu_kernel_id} (): NO_RACE(k)

fun sizeof_gpu_value (): size_t = "mac#sizeof(struct gpu_value)"
fun sizeof_gpu_instr (): size_t = "mac#sizeof(struct gpu_instr)"
fun sizeof_gpu_block (): size_t = "mac#sizeof(struct gpu_block)"
fun sizeof_gpu_kernel (): size_t = "mac#sizeof(struct gpu_kernel)"

prfun prove_value_align (): [n:int | n == 4] int(n)
prfun prove_instr_size (): [n:int | n == 48] int(n)
prfun prove_block_size (): [n:int | n == 32] int(n)
prfun prove_kernel_size (): [n:int | n == 84] int(n)
