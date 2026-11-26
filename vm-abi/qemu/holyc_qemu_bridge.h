// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef HOLY_C_QEMU_BRIDGE_H
#define HOLY_C_QEMU_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

/* Port I/O */
uint8_t  holyc_InU8(uint16_t port);
void     holyc_OutU8(uint16_t port, uint8_t val);
uint16_t holyc_InU16(uint16_t port);
void     holyc_OutU32(uint16_t port, uint32_t val);

/* Execution & control registers */
void     holyc_CLI(void);
void     holyc_STI(void);
void     holyc_HLT(void);
void     holyc_WriteCR3(uint64_t cr3);

/* IDT / IRQ / trap hooks */
void     holyc_IntEntrySet(uint8_t vector, void *handler);
bool     holyc_DispatchIRQ(uint8_t vector);
void     holyc_install_traps(void);
void     holyc_irq_trampoline(void (*isr)(void));

/* Bridge activation & state snapshot (wired build only) */
void     holyc_bridge_activate(void);
void     holyc_bridge_state_snapshot(void **env, uint64_t *base, bool *active,
                                     uint64_t *irq_count, uint64_t *last_cmd,
                                     uint64_t *last_status);

#endif /* HOLY_C_QEMU_BRIDGE_H */
