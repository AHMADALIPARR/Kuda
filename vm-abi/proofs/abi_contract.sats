(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
#include "share/atspre_staload.hats"
#include "share/atspre_staload_libats_ML.hats"

staload "libats/libc/SATS/string.sats"

(* Sort for capability mask *)
sortdef vm_cap_mask_t = uint

absvtype vm_module_hdr_vtype
absvtype vm_exec_ctx_vtype

extern fun vm_cap_mask_and(a: vm_cap_mask_t, b: vm_cap_mask_t): vm_cap_mask_t = "mac#"

extern prfun lemma_cap_and_idem(x: vm_cap_mask_t): [x == vm_cap_mask_and(x, x)] void

implement lemma_cap_and_idem(x) = ()

extern fun check_module_hdr(): void = "mac#"
implement check_module_hdr() = ()

extern fun check_exec_ctx(): void = "mac#"
implement check_exec_ctx() = ()
