(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
// FILE: proofs/abi_contract.dats
(* Implementation of ATS2 proofs for VM ABI v2 *)
staload "./abi_contract.sats"
staload "prelude/DATS/integer.dats"
staload "prelude/DATS/pointer.dats"

(* Proof functions are implemented with [primplement]; [implement] is
 * for non-proof dynamic functions only. *)
primplement prove_align16_region () = align16_lemma{vm_region_t}()
primplement prove_align16_hdr () = align16_lemma{vm_module_hdr_t}()
primplement prove_align16_ctx () = align16_lemma{vm_exec_ctx_t}()
primplement prove_align16_trace () = align16_lemma{vm_trace_rec_t}()

primplement prove_size_region () = 32
primplement prove_size_hdr () = 64
primplement prove_size_ctx () = 176
primplement prove_size_trace () = 48
