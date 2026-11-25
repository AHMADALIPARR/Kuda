(* Copyright (C) 2026 AHMADALIPARR
   SPDX-License-Identifier: AGPL-3.0-only *)
#include "share/atspre_staload.hats"
#include "share/atspre_staload_libats_ML.hats"

staload "libats/SATS/array.sats"
staload "libats/DATS/array.dats"

extern fun array_get_at_size{a:t@ype}{n:int}(A: arrayref(a, n), i: size_t): a = "mac#"
extern fun array_set_at_size{a:t@ype}{n:int}(A: arrayref(a, n), i: size_t, x: a): void = "mac#"

implement main0() = ()
