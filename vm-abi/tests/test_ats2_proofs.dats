(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
(* Compile-time positive and negative ATS2 proof targets *)
staload "../proofs/abi_contract.sats"

implement main0 () = let
  prval pf1 = prove_align16_region()
  prval pf2 = prove_align16_hdr()
  prval pf3 = prove_align16_ctx()
  prval pf4 = prove_align16_trace()
  val s1 = prove_size_region()
  val s2 = prove_size_hdr()
  val s3 = prove_size_ctx()
  val s4 = prove_size_trace()
in
  assertloc(s1 = 32);
  assertloc(s2 = 64);
  assertloc(s3 = 128);
  assertloc(s4 = 48);
  print "ATS2 proofs OK\n"
end
