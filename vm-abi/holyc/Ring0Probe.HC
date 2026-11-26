// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// Ring0Probe.HC — Direct Port I/O, Inline x86-64 ASM, Raw VGA Memory, & IDT Hook

#define VGA_TEXT_BASE 0xB8000
#define PIT_CTRL_PORT 0x43
#define PIT_CH2_PORT  0x42
#define PCSPKR_PORT   0x61

// 1. Raw Memory Poking: Direct write to VGA 80x25 text buffer (Attribute 0x0A = Light Green on Black)
U0 VgaPutStr(I64 row, I64 col, U8 *str, U8 attr=0x0A) {
  U16 *vga = VGA_TEXT_BASE(U16 *);
  I64 idx = row * 80 + col;
  while (*str) {
    vga[idx++] = (*str++) | (attr << 8);
  }
}

// 2. Direct Port I/O: PC Speaker square-wave tone via PIT Channel 2 (ports 0x43, 0x42, 0x61)
U0 BeepRaw(U32 freq_hz) {
  U32 div = 1193180 / freq_hz;
  OutU8(PIT_CTRL_PORT, 0xB6);           // Channel 2, lobyte/hibyte, square wave
  OutU8(PIT_CH2_PORT, div & 0xFF);      // Low byte
  OutU8(PIT_CH2_PORT, (div >> 8) & 0xFF); // High byte
  U8 tmp = InU8(PCSPKR_PORT);
  OutU8(PCSPKR_PORT, tmp | 0x03);       // Gate speaker + PIT ch2
}

U0 BeepOff() {
  OutU8(PCSPKR_PORT, InU8(PCSPKR_PORT) & 0xFC);
}

// 3. Inline x86-64 Assembly: Read CR0 & CR3 control registers directly in Ring 0
U64 ReadCR0() {
  U64 val;
  asm {
    MOV RAX, CR0
    MOV &val[RBP], RAX
  }
  return val;
}

U64 ReadCR3() {
  U64 val;
  asm {
    MOV RAX, CR3
    MOV &val[RBP], RAX
  }
  return val;
}

// 4. Interrupts & IRQs: Minimal custom ISR + IDT registration via IntEntrySet
interrupt U0 CustomIntHandler() {
  // Acknowledge PIC EOI if hardware IRQ, or perform atomic counter increment
  *(0xB8000(U16 *) + 79) = '!' | (0x4F << 8);
}

U0 Ring0Probe() {
  VgaPutStr(0, 0, "RING 0 FLAT IDENTITY-MAPPED SPACE ACTIVE", 0x0E);
  "CR0 = 0x%016X | CR3 = 0x%016X\n", ReadCR0, ReadCR3;

  // Hook software vector 0x80 in the live IDT
  IntEntrySet(0x80, &CustomIntHandler);
  asm {
    INT 0x80
  }
}

Ring0Probe;
