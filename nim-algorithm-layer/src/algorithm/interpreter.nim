# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
import std/[tables, strformat]
import ./ast

type
  ExecResult* = object
    ok*: bool
    error*: string
    environment*: Table[string, int64]

proc eval(expr: Expr, env: Table[string, int64]): tuple[ok: bool, value: int64, error: string] =
  if expr.isNil: return (false, 0'i64, "Null expression")
  case expr.kind
  of ekInteger:
    return (true, expr.intValue, "")
  of ekVariable:
    if env.hasKey(expr.name): return (true, env[expr.name], "")
    return (false, 0'i64, "Undefined variable: " & expr.name)
  of ekBinary:
    let a = eval(expr.left, env)
    if not a.ok: return a
    let b = eval(expr.right, env)
    if not b.ok: return b
    var value: int64
    case expr.op
    of boAdd:
      if (b.value > 0 and a.value > high(int64) - b.value) or
         (b.value < 0 and a.value < low(int64) - b.value):
        return (false, 0'i64, "Signed addition overflow")
      value = a.value + b.value
    of boSub:
      if (b.value < 0 and a.value > high(int64) + b.value) or
         (b.value > 0 and a.value < low(int64) + b.value):
        return (false, 0'i64, "Signed subtraction overflow")
      value = a.value - b.value
    of boMul:
      if a.value != 0 and b.value != 0:
        if (a.value == low(int64) and b.value == -1) or
           (b.value == low(int64) and a.value == -1):
          return (false, 0'i64, "Signed multiplication overflow")
        if a.value > 0:
          if (b.value > 0 and a.value > high(int64) div b.value) or
             (b.value < 0 and b.value < low(int64) div a.value):
            return (false, 0'i64, "Signed multiplication overflow")
        else:
          if (b.value > 0 and a.value < low(int64) div b.value) or
             (b.value < 0 and a.value < high(int64) div b.value):
            return (false, 0'i64, "Signed multiplication overflow")
        value = a.value * b.value
      else:
        value = 0
    return (true, value, "")

proc execute*(program: Program): ExecResult =
  var env = initTable[string, int64]()
  for stmt in program:
    case stmt.kind
    of skLet:
      let value = eval(stmt.letValue, env)
      if not value.ok: return ExecResult(ok: false, error: value.error)
      env[stmt.letName] = value.value
    of skPrint:
      let value = eval(stmt.printValue, env)
      if not value.ok: return ExecResult(ok: false, error: value.error)
      echo value.value
    of skExpression:
      let value = eval(stmt.expression, env)
      if not value.ok: return ExecResult(ok: false, error: value.error)
      discard value.value
  ExecResult(ok: true, environment: env)
