# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
import algorithm/[parser, interpreter, ir]
let parsed = parseProgram("let a = 20; let b = 22; print(a + b);")
doAssert parsed.ok, parsed.error
let lowered = lowerToIr(parsed.program)
doAssert lowered.len > 0
let executed = execute(parsed.program)
doAssert executed.ok, executed.error
doAssert executed.environment["a"] == 20
doAssert executed.environment["b"] == 22
echo "language tests passed"
