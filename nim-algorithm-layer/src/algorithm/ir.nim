# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
import std/[strutils]
import ./ast

type IrOp* = enum
  irConstI64, irLoadLocal, irAddI64, irSubI64, irMulI64, irStoreLocal, irPrintI64
type IrInstruction* = object
  op*: IrOp
  operand*: string

proc emitExpr(expr: Expr, outCode: var seq[IrInstruction]) =
  case expr.kind
  of ekInteger:
    outCode.add(IrInstruction(op: irConstI64, operand: $expr.intValue))
  of ekVariable:
    outCode.add(IrInstruction(op: irLoadLocal, operand: expr.name))
  of ekBinary:
    emitExpr(expr.left, outCode)
    emitExpr(expr.right, outCode)
    let op = case expr.op
      of boAdd: irAddI64
      of boSub: irSubI64
      of boMul: irMulI64
    outCode.add(IrInstruction(op: op, operand: ""))

proc lowerToIr*(program: Program): seq[IrInstruction] =
  for stmt in program:
    case stmt.kind
    of skLet:
      emitExpr(stmt.letValue, result)
      result.add(IrInstruction(op: irStoreLocal, operand: stmt.letName))
    of skPrint:
      emitExpr(stmt.printValue, result)
      result.add(IrInstruction(op: irPrintI64, operand: ""))
    of skExpression:
      emitExpr(stmt.expression, result)

proc `$`*(instruction: IrInstruction): string =
  $instruction.op & (if instruction.operand.len > 0: " " & instruction.operand else: "")
