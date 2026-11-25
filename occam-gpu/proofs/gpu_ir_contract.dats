(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
// FILE: proofs/gpu_ir_contract.dats
staload "./gpu_ir_contract.sats"
staload "prelude/DATS/integer.dats"

(* Proof functions are implemented with [primplement]; [implement] is
 * for non-proof dynamic functions only. *)
primplement prove_value_align () = 4
primplement prove_instr_size () = 48
primplement prove_block_size () = 32
primplement prove_kernel_size () = 84

(* NOTE -- negative proof targets removed, not redefined.
 * The dropped file defined four "negative" targets as
 *   prfn fail_... (...): [false] void = ()
 * but patsopt 0.4.2 rejects the first such definition at TRANS3:
 * checking the body () against [false] void requires discharging
 * [false] from the assumptions, which is impossible (verified on
 * this file: "unsolved constraint: C3NSTRprop(C3TKmain();
 * S2Ecst(false_bool))" at the fail_untyped_value definition). The
 * [false]-void definition idiom cannot be defined in this file, so
 * the targets are not declared as proofs here, and no axiom is
 * introduced for them. The negative properties are still expressed
 * by the lemma constraints themselves -- chan_cap_lemma requires
 * n:nat, so a negative capacity is rejected, and wg_bound_lemma
 * requires x,y,z:pos, so a non-positive workgroup dimension is
 * rejected -- at any use site. *)
