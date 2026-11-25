(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
staload "./gpu_ir_contract.sats"
staload "prelude/DATS/integer.dats"

implement prove_value_align () = 16
implement prove_instr_size () = 32
implement prove_block_size () = 64
implement prove_kernel_size () = 128

prfn fail_untyped_value {v:gpu_value_t} (): [false] void = ()
prfn fail_non_ssa {b:gpu_block_t} (): [false] void = ()
prfn fail_overcap_chan {c:csp_chan_t; n:int | n < 0} (): [false] void = ()
prfn fail_unbounded_wg {k:gpu_kernel_t} (): [false] void = ()
