(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
// FILE: tests/test_ats2_gpu.dats
staload "../proofs/gpu_ir_contract.sats"

implement main0 () = let
  val a = prove_value_align()
  val s1 = prove_instr_size()
  val s2 = prove_block_size()
  val s3 = prove_kernel_size()
in
  assertloc(a >= 16);
  assertloc(s1 = 32);
  assertloc(s2 = 64);
  assertloc(s3 = 128);
  print "ATS2 GPU IR proofs OK\n"
end
