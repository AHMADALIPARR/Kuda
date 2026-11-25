# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
type
  ExprKind* = enum
    ekInteger, ekVariable, ekBinary
  BinaryOp* = enum
    boAdd, boSub, boMul
  Expr* = ref object
    case kind*: ExprKind
    of ekInteger:
      intValue*: int64
    of ekVariable:
      name*: string
    of ekBinary:
      op*: BinaryOp
      left*, right*: Expr
  StmtKind* = enum
    skLet, skPrint, skExpression
  Stmt* = object
    case kind*: StmtKind
    of skLet:
      letName*: string
      letValue*: Expr
    of skPrint:
      printValue*: Expr
    of skExpression:
      expression*: Expr
  Program* = seq[Stmt]
