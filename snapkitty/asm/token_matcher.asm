; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
; Assembly Bite: Literal Token Sequence Matcher
; Matches a fixed pattern of token IDs against the input sequence
; Sliding window — no regex, no invented tools
; Author: Ahmad Ali Parr · Trust: Bel Esprit D'Accord Irrevocable Trust

.DATA
MAX_SEQ_LEN: .word 128
PATTERN_LEN: .word 3
INPUT_TOKENS: .repl 0, MAX_SEQ_LEN
PATTERN:      .repl 101, 2023, 102   ; [CLS] "hello" [SEP] (BERT-style)
MATCH_FLAG:   .word 0
SEQ_PTR:      .word 0
PAT_IDX:      .word 0

.CODE
MAIN:
    LOAD R0, #0
    STORE SEQ_PTR, R0
    LOAD R0, #0
    STORE PAT_IDX, R0
    LOAD R0, #0
    STORE MATCH_FLAG, R0
    CALL MATCH_SEQUENCE
    RET

MATCH_SEQUENCE:
    LOAD R0, SEQ_PTR
    LOAD R1, MAX_SEQ_LEN
    SUB R1, R1, PATTERN_LEN
    CMP R0, R1
    JGT MATCH_END

    LOAD R2, #0
CHECK_LOOP:
    LOAD R3, PAT_IDX
    CMP R3, PATTERN_LEN
    JGE MATCH_FOUND

    LOAD R4, SEQ_PTR
    ADD R4, R4, PAT_IDX
    MUL R4, R4, #4
    LOAD R5, INPUT_TOKENS+R4

    LOAD R6, PATTERN
    MUL R6, R6, PAT_IDX
    MUL R6, R6, #4
    LOAD R7, PATTERN+R6

    CMP R5, R7
    JNE RESET_AND_SHIFT

    LOAD R8, PAT_IDX
    ADD R8, R8, #1
    STORE PAT_IDX, R8
    JMP CHECK_LOOP

RESET_AND_SHIFT:
    LOAD R0, #0
    STORE PAT_IDX, R0
    LOAD R9, SEQ_PTR
    ADD R9, R9, #1
    STORE SEQ_PTR, R9
    JMP MATCH_SEQUENCE

MATCH_FOUND:
    LOAD R0, #1
    STORE MATCH_FLAG, R0
    RET

MATCH_END:
    RET
