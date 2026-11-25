(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
(* vec_add_verify.dats *)
staload "./vec_add_verify.sats"

implement vec_add_verified
  {l_out, l_a, l_b} {n} {tid}
  (pf_out, pf_a, pf_b | p_out, p_a, p_b, tid, n) =
  let
    (* Take out a[tid]: the .sats constraint tid < n proves
       the access is in bounds; array_ptr_takeout returns the
       element view float @ (l_a + tid*sizeof(float)) together
       with a linear proof function restoring the full array view. *)
    val (pf_a_tid, fpf_a | p_a_tid) =
      array_ptr_takeout<float> (pf_a | p_a, tid)
    val va = !p_a_tid
    prval () = pf_a := fpf_a (pf_a_tid)
    (* Take out b[tid] the same way. *)
    val (pf_b_tid, fpf_b | p_b_tid) =
      array_ptr_takeout<float> (pf_b | p_b, tid)
    val vb = !p_b_tid
    prval () = pf_b := fpf_b (pf_b_tid)
    (* Take out out[tid] and store va + vb through the element view. *)
    val (pf_out_tid, fpf_out | p_out_tid) =
      array_ptr_takeout<float> (pf_out | p_out, tid)
    val () = !p_out_tid := va + vb
    prval () = pf_out := fpf_out (pf_out_tid)
  in
    (* Linear views preserved; no aliasing or out-of-bounds access *)
  end
