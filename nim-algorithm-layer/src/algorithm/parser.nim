# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
import std/[strutils]
import ./[ast, lexer]
type
  ParseResult* = object
    ok*: bool
    program*: Program
    error*: string
  ParserState = object
    tokens: seq[Token]
    pos: int
    error: string

proc current(p: ParserState): Token = p.tokens[p.pos]
proc at(p: ParserState, k: TokenKind): bool = p.current.kind == k
proc advance(p: var ParserState): Token =
  result = p.current
  if p.pos < p.tokens.high: inc p.pos
proc fail(p: var ParserState, message: string) =
  if p.error.len == 0:
    let t = p.current
    p.error = message & " at " & $t.line & ":" & $t.column
proc expect(p: var ParserState, k: TokenKind, message: string): bool =
  if p.at(k):
    discard p.advance()
    true
  else:
    p.fail(message)
    false

proc parseExpr(p: var ParserState): Expr

proc parsePrimary(p: var ParserState): Expr =
  if p.at(tkInteger):
    let t = p.advance()
    try:
      return Expr(kind: ekInteger, intValue: parseBiggestInt(t.lexeme).int64)
    except ValueError:
      p.fail("Integer literal out of range")
      return nil
  if p.at(tkIdent):
    let name = p.advance().lexeme
    if p.at(tkLParen):
      p.fail("Function calls are not part of the initial expression grammar")
      return nil
    return Expr(kind: ekVariable, name: name)
  if p.at(tkLParen):
    discard p.advance()
    result = p.parseExpr()
    if not p.expect(tkRParen, "Expected ')'"): return nil
    return
  p.fail("Expected integer, variable, or parenthesized expression")
  nil

proc parseProduct(p: var ParserState): Expr =
  result = p.parsePrimary()
  while result != nil and p.at(tkStar):
    discard p.advance()
    let rhs = p.parsePrimary()
    if rhs == nil: return nil
    result = Expr(kind: ekBinary, op: boMul, left: result, right: rhs)

proc parseExpr(p: var ParserState): Expr =
  result = p.parseProduct()
  while result != nil and (p.at(tkPlus) or p.at(tkMinus)):
    let opToken = p.advance()
    let rhs = p.parseProduct()
    if rhs == nil: return nil
    let op = if opToken.kind == tkPlus: boAdd else: boSub
    result = Expr(kind: ekBinary, op: op, left: result, right: rhs)

proc parseProgram*(source: string): ParseResult =
  let scanned = lex(source)
  if not scanned.ok: return ParseResult(ok: false, error: scanned.error)
  var p = ParserState(tokens: scanned.tokens, pos: 0)
  var statements: Program = @[]
  while not p.at(tkEof) and p.error.len == 0:
    if p.at(tkLet):
      discard p.advance()
      if not p.at(tkIdent):
        p.fail("Expected identifier after 'let'")
        break
      let name = p.advance().lexeme
      if not p.expect(tkEqual, "Expected '=' after binding name"): break
      let value = p.parseExpr()
      if value == nil: break
      statements.add(Stmt(kind: skLet, letName: name, letValue: value))
    elif p.at(tkPrint):
      discard p.advance()
      let value = p.parseExpr()
      if value == nil: break
      statements.add(Stmt(kind: skPrint, printValue: value))
    else:
      let value = p.parseExpr()
      if value == nil: break
      statements.add(Stmt(kind: skExpression, expression: value))
    if p.at(tkSemicolon):
      discard p.advance()
    elif not p.at(tkEof):
      p.fail("Expected ';' between statements")
  if p.error.len > 0:
    return ParseResult(ok: false, error: p.error)
  ParseResult(ok: true, program: statements)
