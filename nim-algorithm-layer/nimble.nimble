# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
version       = "0.1.0"
author        = "SnapKitty Collective"
description   = "Algorithm parser, interpreter, canonical IR, and C11/HolyC bridge boundary"
license       = "MIT"
srcDir        = "src"
bin           = @["nim_algorithm"]
requires "nim >= 1.6.0"

# `nimble test` compiles tests with `--path:.` only, so `algorithm/...`
# (which lives under srcDir "src") is not on the import path. The path
# belongs to the package definition, not the test: run the test with
# src on the path instead of rewriting the import to a relative path.
task test, "Run the language tests":
  exec "nim c --hints:off -r --path:src tests/test_language.nim"
