(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
(* vec_add_verify.dats *)
staload "./vec_add_verify.sats"

implement vec_add_verified
  {l_out, l_a, l_b} {n} {tid}
  (pf_out, pf_a, pf_b | p_out, p_a, p_b, tid, n) =
  let
    val va = array_get_at_size(p_a, tid)
    val vb = array_get_at_size(p_b, tid)
    val () = array_set_at_size(p_out, tid, va + vb)
  in
    (* Linear views preserved; no aliasing or out-of-bounds access *)
  end
