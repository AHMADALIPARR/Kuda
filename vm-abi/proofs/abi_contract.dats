(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
(* Implementation of ATS2 proofs *)
staload "./abi_contract.sats"
staload "prelude/DATS/integer.dats"
staload "prelude/DATS/pointer.dats"

implement prove_align16_region () = align16_lemma{vm_region_t}()
implement prove_align16_hdr () = align16_lemma{vm_module_hdr_t}()
implement prove_align16_ctx () = align16_lemma{vm_exec_ctx_t}()
implement prove_align16_trace () = align16_lemma{vm_trace_rec_t}()

implement prove_size_region () = 32
implement prove_size_hdr () = 64
implement prove_size_ctx () = 128
implement prove_size_trace () = 48

(* Negative proof targets for test suite *)
prfn fail_null_region {p:addr | p == null} (): [false] void = ()
prfn fail_unaligned {a:t@ype} (): [false] void = ()
prfn fail_cap_missing {m:vm_cap_mask_t; r:vm_cap_mask_t}
  (pf: (m land r) != r): [false] void = ()
