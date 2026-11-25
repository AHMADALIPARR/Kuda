# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
import std/[strutils]
type
  TokenKind* = enum
    tkEof, tkIdent, tkInteger, tkLet, tkPrint,
    tkPlus, tkMinus, tkStar, tkEqual, tkLParen, tkRParen, tkComma, tkSemicolon
  Token* = object
    kind*: TokenKind
    lexeme*: string
    line*, column*: int
  LexResult* = object
    ok*: bool
    tokens*: seq[Token]
    error*: string

proc lex*(source: string): LexResult =
  var i = 0
  var line = 1
  var col = 1
  var outTokens: seq[Token] = @[]
  template add(k: TokenKind, s: string, l: int, c: int) =
    outTokens.add(Token(kind: k, lexeme: s, line: l, column: c))
  while i < source.len:
    let ch = source[i]
    if ch in {' ', '\t', '\r'}:
      inc i; inc col
    elif ch == '\n':
      inc i; inc line; col = 1
    elif ch == '#':
      while i < source.len and source[i] != '\n':
        inc i; inc col
    elif ch.isAlphaAscii or ch == '_':
      let start = i
      let startCol = col
      while i < source.len and (source[i].isAlphaNumeric or source[i] == '_'):
        inc i; inc col
      let word = source[start ..< i]
      let kind = case word
        of "let": tkLet
        of "print": tkPrint
        else: tkIdent
      add(kind, word, line, startCol)
    elif ch.isDigit:
      let start = i
      let startCol = col
      while i < source.len and source[i].isDigit:
        inc i; inc col
      add(tkInteger, source[start ..< i], line, startCol)
    else:
      let startCol = col
      let kind = case ch
        of '+': tkPlus
        of '-': tkMinus
        of '*': tkStar
        of '=': tkEqual
        of '(': tkLParen
        of ')': tkRParen
        of ',': tkComma
        of ';': tkSemicolon
        else: tkEof
      if kind == tkEof:
        return LexResult(ok: false, error: "Unexpected character '" & $ch &
          "' at " & $line & ":" & $col)
      add(kind, $ch, line, startCol)
      inc i; inc col
  outTokens.add(Token(kind: tkEof, lexeme: "", line: line, column: col))
  LexResult(ok: true, tokens: outTokens)
