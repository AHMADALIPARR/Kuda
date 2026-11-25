(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
(* vec_add_verify.sats *)
staload "prelude/SATS/array.sats"

fun vec_add_verified
  {l_out, l_a, l_b: addr} {n: nat} {tid: nat | tid < n}
  ( pf_out: !array_v(float, l_out, n)
  , pf_a: !array_v(float, l_a, n)
  , pf_b: !array_v(float, l_b, n)
  | p_out: ptr l_out
  , p_a: ptr l_a
  , p_b: ptr l_b
  , tid: size_t tid
  , n: size_t n
  ) : void
