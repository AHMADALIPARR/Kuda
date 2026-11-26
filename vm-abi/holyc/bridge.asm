; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
; bridge.asm — x86-64 assembly bridge using a shared physical mailbox
BITS 64
DEFAULT REL

%define MAILBOX_BASE 0x00090000
%define OFF_CMD      0x00        ; 0 = idle, 1 = add64, 2 = xor_acc
%define OFF_ARG_A    0x08        ; U64 operand A
%define OFF_ARG_B    0x10        ; U64 operand B
%define OFF_RESULT   0x18        ; U64 return value
%define OFF_STATUS   0x20        ; 0 = pending, 1 = done

global BridgePollStep

section .text
BridgePollStep:
    mov     rbx, MAILBOX_BASE
    mov     rax, [rbx + OFF_CMD]
    test    rax, rax
    jz      .done

    cmp     rax, 1
    je      .do_add
    cmp     rax, 2
    je      .do_xor
    jmp     .clear_cmd

.do_add:
    mov     rcx, [rbx + OFF_ARG_A]
    add     rcx, [rbx + OFF_ARG_B]
    mov     [rbx + OFF_RESULT], rcx
    jmp     .mark_ready

.do_xor:
    mov     rcx, [rbx + OFF_ARG_A]
    xor     rcx, [rbx + OFF_ARG_B]
    mov     [rbx + OFF_RESULT], rcx
    jmp     .mark_ready

.mark_ready:
    mov     qword [rbx + OFF_STATUS], 1

.clear_cmd:
    mov     qword [rbx + OFF_CMD], 0

.done:
    ret
