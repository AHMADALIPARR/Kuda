// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <stdint.h>
#include <stddef.h>

typedef struct isel_rule_s {
    uint32_t opcode;
    uint32_t target_op;
} isel_rule_t;

static const isel_rule_t ptx_rules[] = {
    { 0x01, 0x100 },
    { 0x02, 0x101 },
    { 0, 0 }
};

static const isel_rule_t amdgpu_rules[] = {
    { 0x01, 0x200 },
    { 0x02, 0x201 },
    { 0, 0 }
};

__attribute__((used))
static const isel_rule_t *get_ptx_rules(void) { return ptx_rules; }

__attribute__((used))
static const isel_rule_t *get_amdgpu_rules(void) { return amdgpu_rules; }

int instr_select(int target, uint32_t opcode) {
    const isel_rule_t *rules = (target == 0) ? get_ptx_rules() : get_amdgpu_rules();
    for (size_t i = 0; rules[i].opcode != 0; i++) {
        if (rules[i].opcode == opcode)
            return (int)rules[i].target_op;
    }
    return -1;
}
