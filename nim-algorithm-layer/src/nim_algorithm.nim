# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
import std/[os, strutils]
import algorithm/[parser, interpreter, ir]

proc usage() =
  echo "nim_algorithm <file.alg> [--emit-ir]"
  echo "Language: let name = integer; add(a, b); sub(a, b); mul(a, b); print(expr)"

proc main() =
  if paramCount() < 1:
    usage()
    quit(2)
  let source = readFile(paramStr(1))
  let parsed = parseProgram(source)
  if not parsed.ok:
    stderr.writeLine(parsed.error)
    quit(1)
  if paramCount() >= 2 and paramStr(2) == "--emit-ir":
    let lowered = lowerToIr(parsed.program)
    for instruction in lowered:
      echo instruction
    return
  let result = execute(parsed.program)
  if not result.ok:
    stderr.writeLine(result.error)
    quit(1)

when isMainModule:
  main()
