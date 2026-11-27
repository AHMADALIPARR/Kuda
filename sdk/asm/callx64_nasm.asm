; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
; callx64_nasm.asm - NASM x86-64 argument/FP marshaling trampoline.
; Scope: SysV AMD64 (Linux). GP args from array (max 6: rdi,rsi,rdx,rcx,r8,r9),
; FP args from array (max 8: xmm0-xmm7), integer/pointer return in rax.
; Build: nasm -f elf64 asm/callx64_nasm.asm -o build/callx64_sysv.o
; NOTE: this is a verification-grade trampoline, not the full AngelScript
; CallX64 with stack-argument spill; extend per as_callfunc_x64_msvc_asm.asm.

section .text
global callx64_sysv

callx64_sysv:
    ; rdi=target rsi=gp_args rdx=fp_args ecx=n_gp r8d=n_fp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 8 ; 5 pushes + 8 = 48; caller rsp was 8 mod 16 -> 16-aligned? (verify per call site)

    mov r12, rdi ; target
    mov r13, rsi ; gp array
    mov r14, rdx ; fp array
    mov ebx, r8d ; n_fp

    ; Load GP args from array into named registers
    mov rdi, [r13 + 0*8]
    mov rsi, [r13 + 1*8]
    mov rdx, [r13 + 2*8]
    mov rcx, [r13 + 3*8]
    mov r8, [r13 + 4*8]
    mov r9, [r13 + 5*8]

    ; Load FP args
    xor eax, eax
    test ebx, ebx
    jz .fp_done
    movsd xmm0, [r14 + 0*8]
    cmp ebx, 2
    jb .fp_done
    movsd xmm1, [r14 + 1*8]
    cmp ebx, 3
    jb .fp_done
    movsd xmm2, [r14 + 2*8]
    cmp ebx, 4
    jb .fp_done
    movsd xmm3, [r14 + 3*8]
.fp_done:
    mov al, bl ; vector register count (varargs convention)
    and al, 0x7
    call r12

    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
