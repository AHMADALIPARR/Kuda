// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "qemu/osdep.h"
#include "holyc_qemu_bridge.h"
#include "exec/address-spaces.h"
#include "exec/memory.h"
#include "qemu/main-loop.h"
#include "qemu/timer.h"
#include "target/i386/cpu.h"

#if defined(_WIN32)
#include <windows.h>
#define HOLYC_ABI __attribute__((sysv_abi)) // Forces GCC/Clang on Windows to match Linux SysV JIT ABI
#else
#include <signal.h>
#include <ucontext.h>
#define HOLYC_ABI
#endif

#if !defined(bql_lock)
static inline void holyc_bql_lock(void)
{
    qemu_mutex_lock_iothread();
}
static inline void holyc_bql_unlock(void)
{
    qemu_mutex_unlock_iothread();
}
#define bql_lock()   holyc_bql_lock()
#define bql_unlock() holyc_bql_unlock()
#endif

CPUX86State *global_holyc_env;

/* Internal bridge state (static): mailbox base, active flag, IRQ counter,
   and last-command/status tracking. global_holyc_env stays CPUX86State *. */
static struct {
    hwaddr mailbox_base;
    bool active;
    bool verbose;
    bool traps_installed;
    uint64_t irq_count;
    uint64_t last_cmd;
    uint64_t last_status;
} holyc_bridge_state = {
    .mailbox_base = 0x90000,
    .active = false,
    .verbose = true,
    .traps_installed = false,
    .irq_count = 0,
    .last_cmd = 0,
    .last_status = 0,
};

static void (*holyc_idt[256])(void);

// ============================================================================
// 1. DIRECT PORT I/O (Locked with BQL for thread safety across Linux & Win32)
// ============================================================================

HOLYC_ABI uint8_t holyc_InU8(uint16_t port) {
    uint8_t val = 0;
    bql_lock();

    address_space_read(&address_space_io, port, MEMTXATTRS_UNSPECIFIED, &val, 1);
    bql_unlock();
    return val;
}

HOLYC_ABI void holyc_OutU8(uint16_t port, uint8_t val) {
    bql_lock();

    address_space_write(&address_space_io, port, MEMTXATTRS_UNSPECIFIED, &val, 1);
    bql_unlock();
}

HOLYC_ABI uint16_t holyc_InU16(uint16_t port) {
    uint16_t val = 0;
    bql_lock();

    address_space_read(&address_space_io, port, MEMTXATTRS_UNSPECIFIED, &val, 2);
    bql_unlock();
    return val;
}

HOLYC_ABI void holyc_OutU32(uint16_t port, uint32_t val) {
    bql_lock();

    address_space_write(&address_space_io, port, MEMTXATTRS_UNSPECIFIED, &val, 4);
    bql_unlock();
}

// ============================================================================
// 2. PRIVILEGED INSTRUCTIONS & CONTROL REGISTERS
// ============================================================================

HOLYC_ABI void holyc_CLI(void) {
    if (global_holyc_env) global_holyc_env->eflags &= ~IF_MASK;
}

HOLYC_ABI void holyc_STI(void) {
    if (global_holyc_env) global_holyc_env->eflags |= IF_MASK;
}

HOLYC_ABI void holyc_HLT(void) {
    if (global_holyc_env) {

        CPUState *cs = env_cpu(global_holyc_env);
        cs->halted = 1;
        cpu_exit(cs);
    }
}

HOLYC_ABI void holyc_WriteCR3(uint64_t cr3) {
    if (global_holyc_env) {
        bql_lock();
        cpu_x86_update_cr3(global_holyc_env, cr3);
        bql_unlock();
    }
}

// ============================================================================
// 3. INTERRUPTS & IRQs (IntEntrySet + SysV-safe ISR Dispatcher)
// ============================================================================

HOLYC_ABI void holyc_IntEntrySet(uint8_t vector, void *handler) {
    holyc_idt[vector] = (void (*)(void))handler;
}

__attribute__((naked)) static void holyc_call_isr_sysv(void (*isr)(void)) {

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
#if defined(_WIN32)
        "call *%rcx\n\t" // Win64 passes first arg in RCX
#else
        "call *%rdi\n\t" // Linux SysV passes first arg in RDI
#endif

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

bool holyc_DispatchIRQ(uint8_t vector) {
    /* Gate: only route once the bridge is fully initialised. The counter
       is bumped atomically (no BQL needed on this hot path). */
    if (!global_holyc_env || !holyc_bridge_state.active) {
        return false;
    }
    __atomic_add_fetch(&holyc_bridge_state.irq_count, 1, __ATOMIC_RELAXED);
    if (holyc_idt[vector]) {
        holyc_call_isr_sysv(holyc_idt[vector]);
        return true;
    }
    return false;
}

// ============================================================================
// 4. RAW TRAP-AND-EMULATE HANDLER (Linux sigaction + Windows VEH)
// ============================================================================

static bool emulate_ring0_instruction(uint8_t *rip, uint64_t *rax, uint64_t *rdx, uint64_t *rip_reg) {
    switch (*rip) {
        case 0xFA: // CLI
            holyc_CLI();
            *rip_reg += 1;
            return true;
        case 0xFB: // STI
            holyc_STI();
            *rip_reg += 1;
            return true;
        case 0xF4: // HLT
            holyc_HLT();
            *rip_reg += 1;
            return true;
        case 0xEE: // OUT DX, AL
            holyc_OutU8((uint16_t)(*rdx & 0xFFFF), (uint8_t)(*rax & 0xFF));
            *rip_reg += 1;
            return true;
        case 0xEC: // IN AL, DX
            *rax = (*rax & ~0xFFULL) | holyc_InU8((uint16_t)(*rdx & 0xFFFF));
            *rip_reg += 1;
            return true;
        default:
            return false;
    }
}

#if defined(_WIN32)
static LONG CALLBACK holyc_veh_handler(PEXCEPTION_POINTERS ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_PRIV_INSTRUCTION || code == EXCEPTION_ILLEGAL_INSTRUCTION) {
        PCONTEXT ctx = ep->ContextRecord;
        if (emulate_ring0_instruction((uint8_t *)ctx->Rip, &ctx->Rax, &ctx->Rdx, &ctx->Rip)) {
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void holyc_install_traps(void) {
    AddVectoredExceptionHandler(1, holyc_veh_handler);
    holyc_bridge_state.traps_installed = true;
}
#else
static void holyc_sig_handler(int sig, siginfo_t *info, void *ucontext) {

    ucontext_t *uc = (ucontext_t *)ucontext;
    uint64_t *rip = (uint64_t *)&uc->uc_mcontext.gregs[REG_RIP];
    uint64_t *rax = (uint64_t *)&uc->uc_mcontext.gregs[REG_RAX];
    uint64_t *rdx = (uint64_t *)&uc->uc_mcontext.gregs[REG_RDX];

    if (emulate_ring0_instruction((uint8_t *)(*rip), rax, rdx, rip)) {
        return;
    }
    // Unhandled fault: restore default and re-raise
    signal(sig, SIG_DFL);
    raise(sig);
}

void holyc_install_traps(void) {
    struct sigaction sa = {0};
    sa.sa_sigaction = holyc_sig_handler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGILL, &sa, NULL) != 0 ||
        sigaction(SIGSEGV, &sa, NULL) != 0) {
        fprintf(stderr, "holyc-bridge: holyc_install_traps: sigaction failed: %s\n",
                strerror(errno));
        return;
    }
    holyc_bridge_state.traps_installed = true;
}
#endif

// ============================================================================
// 5. BRIDGE ACTIVATION, MAILBOX POLLER & STATE SNAPSHOT (wired build only)
// ============================================================================

/* Mailbox layout (matches BridgeClient.HC / bridge.asm). */
#define HOLYC_MB_OFF_CMD      0x00
#define HOLYC_MB_OFF_ARG_A    0x08
#define HOLYC_MB_OFF_ARG_B    0x10
#define HOLYC_MB_OFF_RESULT   0x18
#define HOLYC_MB_OFF_STATUS   0x20
#define HOLYC_MB_CMD_ADD      1
#define HOLYC_MB_CMD_XOR      2

static QEMUTimer *holyc_mailbox_timer;

static void holyc_mailbox_poll(void)
{
    uint64_t cmd = 0, arg_a = 0, arg_b = 0, result = 0;
    bool took_bql = false;

    if (!holyc_bridge_state.active) {
        return;
    }

    /* Take the BQL if this thread does not already hold it (a timer
       callback normally runs with it held). Every mailbox and
       holyc_bridge_state write below happens under the BQL. */
    if (!qemu_mutex_iothread_locked()) {
        qemu_mutex_lock_iothread();
        took_bql = true;
    }

    address_space_read(&address_space_memory,
                       holyc_bridge_state.mailbox_base + HOLYC_MB_OFF_CMD,
                       MEMTXATTRS_UNSPECIFIED, &cmd, sizeof(cmd));
    if (cmd == 0) {
        goto out;
    }

    if (cmd != HOLYC_MB_CMD_ADD && cmd != HOLYC_MB_CMD_XOR) {
        /* Unknown command: clear it (matches BridgePollStep) and stop. */
        uint64_t zero = 0;
        address_space_write(&address_space_memory,
                            holyc_bridge_state.mailbox_base + HOLYC_MB_OFF_CMD,
                            MEMTXATTRS_UNSPECIFIED, &zero, sizeof(zero));
        goto out;
    }

    address_space_read(&address_space_memory,
                       holyc_bridge_state.mailbox_base + HOLYC_MB_OFF_ARG_A,
                       MEMTXATTRS_UNSPECIFIED, &arg_a, sizeof(arg_a));
    address_space_read(&address_space_memory,
                       holyc_bridge_state.mailbox_base + HOLYC_MB_OFF_ARG_B,
                       MEMTXATTRS_UNSPECIFIED, &arg_b, sizeof(arg_b));

    result = (cmd == HOLYC_MB_CMD_ADD) ? (arg_a + arg_b) : (arg_a ^ arg_b);

    /* Write result, then flip status to done, then clear the command.
       All under the BQL. */
    address_space_write(&address_space_memory,
                        holyc_bridge_state.mailbox_base + HOLYC_MB_OFF_RESULT,
                        MEMTXATTRS_UNSPECIFIED, &result, sizeof(result));
    {
        uint64_t done = 1;
        address_space_write(&address_space_memory,
                            holyc_bridge_state.mailbox_base + HOLYC_MB_OFF_STATUS,
                            MEMTXATTRS_UNSPECIFIED, &done, sizeof(done));
    }
    {
        uint64_t zero = 0;
        address_space_write(&address_space_memory,
                            holyc_bridge_state.mailbox_base + HOLYC_MB_OFF_CMD,
                            MEMTXATTRS_UNSPECIFIED, &zero, sizeof(zero));
    }
    holyc_bridge_state.last_cmd = cmd;
    holyc_bridge_state.last_status = 1;

    if (holyc_bridge_state.verbose) {
        fprintf(stderr,
                "holyc-bridge: mailbox cmd=%" PRIu64 " result=0x%" PRIx64 " status=1\n",
                cmd, result);
    }

out:
    if (took_bql) {
        qemu_mutex_unlock_iothread();
    }
}

static void holyc_mailbox_timer_cb(void *opaque)
{
    (void)opaque;
    holyc_mailbox_poll();
    timer_mod(holyc_mailbox_timer,
              qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + 10);
}

void holyc_bridge_activate(void)
{
    if (holyc_bridge_state.active) {
        return;
    }
    if (!global_holyc_env || !holyc_bridge_state.traps_installed) {
        fprintf(stderr,
                "holyc-bridge: activation failed (env=%p traps_installed=%d); leaving active=false\n",
                (void *)global_holyc_env,
                holyc_bridge_state.traps_installed);
        return;
    }

    if (!qemu_mutex_iothread_locked()) {
        qemu_mutex_lock_iothread();
        holyc_bridge_state.active = true;
        qemu_mutex_unlock_iothread();
    } else {
        holyc_bridge_state.active = true;
    }

    holyc_mailbox_timer = timer_new_ms(QEMU_CLOCK_VIRTUAL,
                                       holyc_mailbox_timer_cb, NULL);
    timer_mod(holyc_mailbox_timer,
              qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + 10);

    if (holyc_bridge_state.verbose) {
        fprintf(stderr,
                "holyc-bridge: init done (env=%p mailbox_base=0x%" PRIx64 " traps_installed, active=true)\n",
                (void *)global_holyc_env,
                (uint64_t)holyc_bridge_state.mailbox_base);
    }
}

void holyc_bridge_state_snapshot(void **env, uint64_t *base, bool *active,
                                 uint64_t *irq_count, uint64_t *last_cmd,
                                 uint64_t *last_status)
{
    *env = (void *)global_holyc_env;
    *base = (uint64_t)holyc_bridge_state.mailbox_base;
    *active = holyc_bridge_state.active;
    *irq_count = __atomic_load_n(&holyc_bridge_state.irq_count,
                                 __ATOMIC_RELAXED);
    *last_cmd = holyc_bridge_state.last_cmd;
    *last_status = holyc_bridge_state.last_status;
}
