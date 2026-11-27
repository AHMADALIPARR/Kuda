; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
; Assembly Bite Macromodel — Multiplicity of Neurons
; Full transformer: 24 layers, 12 heads, 768 dim, M=4 multiplicity copies
; Author: Ahmad Ali Parr · Trust: Bel Esprit D'Accord Irrevocable Trust

.DATA
N:      .word 128       ; sequence length
D:      .word 768       ; model dimension
H:      .word 3072      ; feed-forward hidden
A:      .word 12        ; attention heads
L:      .word 24        ; transformer layers
M:      .word 4         ; multiplicity (copies per neuron)

W_Q:    .repl 0.0, L*A*M*D*D
W_K:    .repl 0.0, L*A*M*D*D
W_V:    .repl 0.0, L*A*M*D*D
W_O:    .repl 0.0, L*A*M*D*D
B_Q:    .repl 0.0, L*A*M*D
B_K:    .repl 0.0, L*A*M*D
B_V:    .repl 0.0, L*A*M*D
B_O:    .repl 0.0, L*A*M*D
W_FF1:  .repl 0.0, L*M*D*H
W_FF2:  .repl 0.0, L*M*H*D
B_FF1:  .repl 0.0, L*M*H
B_FF2:  .repl 0.0, L*M*D

INPUT:  .repl 0.0, N*D
OUTPUT: .repl 0.0, N*D
TMP:    .repl 0.0, N*D*(A*M*2 + H*M)

SQRT_D: .float 27.7128   ; sqrt(768)

.CODE
MAIN:
    LOAD R0, L
LAYER_LOOP:
    CMP R0, #0
    JEQ END_MODEL
    LOAD R1, A
HEAD_LOOP:
    CMP R1, #0
    JEQ NEXT_LAYER
    LOAD R2, M
    LOAD R3, #0
MULT_LOOP:
    CMP R2, #0
    JEQ NEXT_HEAD

    ; Q = INPUT * W_Q[l,h,m] + B_Q[l,h,m]
    LOAD R4, INPUT
    LOAD R5, W_Q
    MUL R6, R0, A*M*D*D
    MUL R7, R1, M*D*D
    MUL R8, R2, D*D
    ADD R5, R5, R6
    ADD R5, R5, R7
    ADD R5, R5, R8
    CALL MATMUL
    LOAD R9, B_Q
    MUL R10, R0, A*M*D
    MUL R11, R1, M*D
    MUL R12, R2, D
    ADD R9, R9, R10
    ADD R9, R9, R11
    ADD R9, R9, R12
    CALL ADD_BIAS
    STORE TMP0, Q_buf

    ; K = INPUT * W_K[l,h,m] + B_K[l,h,m]
    LOAD R4, INPUT
    LOAD R5, W_K
    MUL R6, R0, A*M*D*D
    MUL R7, R1, M*D*D
    MUL R8, R2, D*D
    ADD R5, R5, R6
    ADD R5, R5, R7
    ADD R5, R5, R8
    CALL MATMUL
    LOAD R9, B_K
    MUL R10, R0, A*M*D
    MUL R11, R1, M*D
    MUL R12, R2, D
    ADD R9, R9, R10
    ADD R9, R9, R11
    ADD R9, R9, R12
    CALL ADD_BIAS
    STORE TMP0+(N*D), K_buf

    ; V = INPUT * W_V[l,h,m] + B_V[l,h,m]
    LOAD R4, INPUT
    LOAD R5, W_V
    MUL R6, R0, A*M*D*D
    MUL R7, R1, M*D*D
    MUL R8, R2, D*D
    ADD R5, R5, R6
    ADD R5, R5, R7
    ADD R5, R5, R8
    CALL MATMUL
    LOAD R9, B_V
    MUL R10, R0, A*M*D
    MUL R11, R1, M*D
    MUL R12, R2, D
    ADD R9, R9, R10
    ADD R9, R9, R11
    ADD R9, R9, R12
    CALL ADD_BIAS
    STORE TMP0+(2*N*D), V_buf

    ; Attention scores = (Q * K^T) / sqrt(D)
    LOAD R4, Q_buf
    LOAD R5, K_buf
    CALL MATMUL_TRANSPOSE
    LOAD R6, SQRT_D
    CALL DIV_SCALAR
    CALL SOFTMAX

    ; Attention output = scores * V
    LOAD R4, TMP0
    LOAD R5, V_buf
    CALL MATMUL
    STORE TMP0+(3*N*D), ATTN_buf

    ; Output projection
    LOAD R4, ATTN_buf
    LOAD R5, W_O
    MUL R6, R0, A*M*D*D
    MUL R7, R1, M*D*D
    MUL R8, R2, D*D
    ADD R5, R5, R6
    ADD R5, R5, R7
    ADD R5, R5, R8
    CALL MATMUL
    LOAD R9, B_O
    MUL R10, R0, A*M*D
    MUL R11, R1, M*D
    MUL R12, R2, D
    ADD R9, R9, R10
    ADD R9, R9, R11
    ADD R9, R9, R12
    CALL ADD_BIAS
    STORE TMP0+(3*N*D), ATTN_OUT_buf

    ; Feed-Forward: FF1 = RELU(attn_out * W_FF1 + B_FF1)
    LOAD R4, ATTN_OUT_buf
    LOAD R5, W_FF1
    MUL R6, R0, M*D*H
    MUL R7, R2, D*H
    ADD R5, R5, R6
    ADD R5, R5, R7
    CALL MATMUL
    LOAD R9, B_FF1
    MUL R10, R0, M*H
    MUL R11, R2, H
    ADD R9, R9, R10
    ADD R9, R9, R11
    CALL ADD_BIAS
    CALL RELU
    STORE TMP0, FF1_buf

    ; FF2 = FF1 * W_FF2 + B_FF2
    LOAD R4, FF1_buf
    LOAD R5, W_FF2
    MUL R6, R0, M*H*D
    MUL R7, R2, H*D
    ADD R5, R5, R6
    ADD R5, R5, R7
    CALL MATMUL
    LOAD R9, B_FF2
    MUL R10, R0, M*D
    MUL R11, R2, D
    ADD R9, R9, R10
    ADD R9, R9, R11
    CALL ADD_BIAS
    STORE TMP0+(N*H), FF2_buf

    ; Add & Norm (residual + layer norm)
    LOAD R4, ATTN_OUT_buf
    LOAD R5, FF2_buf
    CALL ADD
    CALL LAYER_NORM
    STORE TMP0, LAYER_OUT_buf

    ; Aggregate multiplicity (sum over copies)
    LOAD R6, LAYER_OUT_buf
    LOAD R7, LAYER_ACC_buf
    CALL ADD
    STORE LAYER_ACC_buf, R6

    SUB R2, #1
    JMP MULT_LOOP

NEXT_HEAD:
    SUB R1, #1
    LOAD R2, M
    JMP HEAD_LOOP

NEXT_LAYER:
    SUB R0, #1
    LOAD R1, A
    JMP LAYER_LOOP

END_MODEL:
    LOAD R4, LAYER_ACC_buf
    STORE OUTPUT, R4
    RET
