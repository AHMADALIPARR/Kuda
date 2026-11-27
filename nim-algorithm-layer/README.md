<!-- Copyright (C) 2026 AHMADALIPARR
SPDX-License-Identifier: AGPL-3.0-only -->
# Nim Algorithm Layer

Nim supplies the algorithm-language front end and development interpreter.
It does not own native memory, proof checking, or final execution.

## Current grammar

```text
program    := statement (";" statement)* ";"?
statement  := "let" IDENT "=" expression
            | "print" expression
            | expression
expression := product (("+" | "-") product)*
product    := primary ("*" primary)*
primary    := INTEGER | IDENT | "(" expression ")"
```

The initial interpreter supports signed 64-bit integer arithmetic and detects
arithmetic overflow. The initial IR is a stack-oriented sequence with explicit
local loads/stores and integer operations.

## Build

Requires Nim >= 1.6.

```sh
nimble build
nim c -r --path:src tests/test_language.nim
./nim_algorithm examples/sum.alg
./nim_algorithm examples/sum.alg --emit-ir
```

## C11 bridge

`algorithm/c11_bridge.nim` declares the narrow tagged-value ABI from the C11
substrate. It is an integration boundary only; it deliberately does not
duplicate C11 ownership or dispatch logic. Adjust the include/library paths
for the actual workspace layout before linking.

The C11 substrate must export `sk_status_string` if that optional diagnostic
binding is used; add its implementation to the C11 library or omit that
binding until implemented.
