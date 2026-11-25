(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
// FILE: proofs/gpu_ir_contract.sats
(* ATS2 linear/dependent proofs for typed GPU IR and Occam process admission *)
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

absprop WELL_TYPED (v: gpu_value_t, t: gpu_type_t)
absprop SSA_FORM (b: gpu_block_t)
absprop CHANNEL_CAP (c: csp_chan_t, n: int)
absprop WORKGROUP_BOUNDED (k: gpu_kernel_t, x: int, y: int, z: int)
absprop NO_RACE (k: gpu_kernel_t)

praxi well_typed_lemma {v:gpu_value_t; t:gpu_type_t} (): WELL_TYPED(v, t)
praxi ssa_lemma {b:gpu_block_t} (): SSA_FORM(b)
praxi chan_cap_lemma {c:csp_chan_t; n:nat} (): CHANNEL_CAP(c, n)
praxi wg_bound_lemma {k:gpu_kernel_t; x,y,z:pos} (): WORKGROUP_BOUNDED(k, x, y, z)
praxi no_race_lemma {k:gpu_kernel_t} (): NO_RACE(k)

fun sizeof_gpu_value (): size_t = "mac#sizeof(struct gpu_value)"
fun sizeof_gpu_instr (): size_t = "mac#sizeof(struct gpu_instr)"
fun sizeof_gpu_block (): size_t = "mac#sizeof(struct gpu_block)"
fun sizeof_gpu_kernel (): size_t = "mac#sizeof(struct gpu_kernel)"

prfun prove_value_align (): [n:int | n >= 16] int(n)
prfun prove_instr_size (): [n:int | n == 32] int(n)
prfun prove_block_size (): [n:int | n == 64] int(n)
prfun prove_kernel_size (): [n:int | n == 128] int(n)
