// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <signal.h>
#include <ucontext.h>
#include "qemu/osdep.h"
#include "holyc_qemu_bridge.h"
#include "exec/address-spaces.h"
#include "target/i386/cpu.h"

extern CPUX86State *global_holyc_env; // Pointer to QEMU's vCPU state

static void G_GNUC_UNUSED holyc_fault_handler(int sig, siginfo_t *info, void *ucontext) {
    ucontext_t *uc = (ucontext_t *)ucontext;
    uint8_t *rip = (uint8_t *)uc->uc_mcontext.gregs[REG_RIP];

    if (sig == SIGILL || sig == SIGSEGV) {
        // 1. Privileged single-byte opcodes (CLI = 0xFA, STI = 0xFB, HLT = 0xF4)
        if (*rip == 0xFA) { // CLI
            global_holyc_env->eflags &= ~IF_MASK;
            uc->uc_mcontext.gregs[REG_RIP] += 1;
            return;
        }
        if (*rip == 0xFB) { // STI
            global_holyc_env->eflags |= IF_MASK;
            uc->uc_mcontext.gregs[REG_RIP] += 1;
            return;
        }
        if (*rip == 0xF4) { // HLT
            CPUState *cs = env_cpu(global_holyc_env);
            cs->halted = 1;
            uc->uc_mcontext.gregs[REG_RIP] += 1;
            cpu_exit(cs);
            return;
        }

        // 2. Port I/O: OUT DX, AL (0xEE) / IN AL, DX (0xEC)
        if (*rip == 0xEE) {
            uint16_t port = uc->uc_mcontext.gregs[REG_RDX] & 0xFFFF;
            uint8_t val   = uc->uc_mcontext.gregs[REG_RAX] & 0xFF;

            address_space_write(&address_space_io, port, MEMTXATTRS_UNSPECIFIED, &val, 1);
            uc->uc_mcontext.gregs[REG_RIP] += 1;
            return;
        }
        if (*rip == 0xEC) {
            uint16_t port = uc->uc_mcontext.gregs[REG_RDX] & 0xFFFF;

            uint8_t val   = 0;
            address_space_read(&address_space_io, port, MEMTXATTRS_UNSPECIFIED, &val, 1);
            uc->uc_mcontext.gregs[REG_RAX] = (uc->uc_mcontext.gregs[REG_RAX] & ~0xFFULL) | val;
            uc->uc_mcontext.gregs[REG_RIP] += 1;
            return;
        }

        // 3. MMIO / VGA Memory Poke (SIGSEGV at info->si_addr)
        // Decode ModR/M at `rip` or route faulting guest address via address_space_rw()
    }
}
#include <stdint.h>

// Port I/O
void     holyc_OutU16(uint16_t port, uint16_t val);
uint32_t holyc_InU32(uint16_t port);

// Control Registers & CPU State
void     holyc_WriteCR0(uint64_t val);
uint64_t holyc_ReadCR0(void);
uint64_t holyc_ReadCR3(void);
// Assembly trampoline: saves SysV volatile registers, calls HolyC ISR, returns
__attribute__((naked)) void holyc_irq_trampoline(void (*isr)(void)) {
    __asm__ volatile (
        "push %rax\n\t"
        "push %rcx\n\t"
        "push %rdx\n\t"
        "push %rsi\n\t"
        "push %rdi\n\t"
        "push %r8\n\t"
        "push %r9\n\t"
        "push %r10\n\t"
        "push %r11\n\t"
        "call *%rdi\n\t"
        "pop %r11\n\t"
        "pop %r10\n\t"
        "pop %r9\n\t"
        "pop %r8\n\t"
        "pop %rdi\n\t"
        "pop %rsi\n\t"
        "pop %rdx\n\t"
        "pop %rcx\n\t"
        "pop %rax\n\t"
        "ret\n\t"
    );
}
