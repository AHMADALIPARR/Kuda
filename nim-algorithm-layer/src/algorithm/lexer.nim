# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
proc isOperator(c: char): bool =
  case c
  of '+', '-', '*', '/', '%': true
  of '=': true
  of '<', '>': true
  else: false

proc nextToken(src: string, pos: var int): string =
  if pos >= src.len: return ""
  let c = src[pos]
  if isOperator(c):
    inc pos
    return $c
  return ""
