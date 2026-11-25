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
