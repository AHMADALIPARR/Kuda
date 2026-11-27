; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
; Assembly Bite: Tree Pattern Matcher — DFS path matching
; Adjacency list: [id, first_child, next_sibling] per node
; Standard DFS with explicit stack — no invented tools
; Author: Ahmad Ali Parr · Trust: Bel Esprit D'Accord Irrevocable Trust

.DATA
MAX_NODES:   .word 100
PATTERN_LEN: .word 3
NODES:       .repl 0, MAX_NODES*3   ; [id, first_child, next_sibling]
PATTERN:     .repl 0, PATTERN_LEN   ; e.g. [1,5,9] root→child→grandchild
MATCH_FLAG:  .word 0
STACK:       .repl 0, 100           ; DFS stack (node_id, pattern_index)
SP:          .word 0

.CODE
MAIN:
    LOAD R0, #0
    STORE SP, R0
    LOAD R0, #0
    STORE MATCH_FLAG, R0
    LOAD R0, #1        ; push root (ID=1)
    STORE STACK+0, R0
    LOAD R0, #0
    STORE STACK+1, R0  ; pattern_idx=0
    LOAD R0, #2
    STORE SP, R0
    CALL DFS_MATCH
    RET

DFS_MATCH:
    LOAD R0, SP
    CMP R0, #0
    JEQ DFS_END

    LOAD R1, SP
    SUB R1, #2
    STORE SP, R1
    LOAD R2, STACK+R1      ; node_id
    LOAD R3, STACK+R1+1    ; pattern_idx

    LOAD R4, PATTERN_LEN
    CMP R3, R4
    JGE MATCH_FOUND

    MUL R5, R2, #3
    LOAD R6, NODES+R5
    CMP R6, R2
    JNE DFS_MATCH

    LOAD R7, PATTERN+R3
    CMP R6, R7
    JNE CHECK_SIBLINGS

    LOAD R8, R3
    ADD R8, #1
    LOAD R9, PATTERN_LEN
    CMP R8, R9
    JGE MATCH_FOUND

    LOAD R10, NODES+R5+1
    CMP R10, #0
    JEQ CHECK_SIBLINGS
    LOAD R11, SP
    STORE STACK+R11, R10
    LOAD STACK+R11+1, R8
    LOAD R12, #2
    ADD R11, R11, R12
    STORE SP, R11
    JMP DFS_MATCH

CHECK_SIBLINGS:
    LOAD R13, NODES+R5+2
    CMP R13, #0
    JEQ DFS_MATCH
    LOAD R14, SP
    STORE STACK+R14, R13
    LOAD STACK+R14+1, R3
    LOAD R15, #2
    ADD R14, R14, R15
    STORE SP, R14
    JMP DFS_MATCH

MATCH_FOUND:
    LOAD R0, #1
    STORE MATCH_FLAG, R0
    RET

DFS_END:
    RET
