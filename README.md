<!-- Copyright (C) 2026 AHMADALIPARR -->
<!-- SPDX-License-Identifier: AGPL-3.0-only -->
# KUDA

A CUDA replacement built on custom GPU / Ring 0 dispatch.

No CUDA. No PyTorch. No Python. AGPLv3 only.

## Layout

```
languages     occam ──┐   Nim ──┐   Mojo kernels ──┐
                      ▼         ▼                  ▼
gpu ir        occam-gpu: GPU IR, PTX / AMDGPU / SPIR-V backends
                      ▼
runtime       native-execution (C11) · snapkitty-heterogeneous runtime
                      ▼
ring 0        vm-abi: HolyC VM core + C11 substrate + ATS2 proofs
                      ▼
execution     vm-abi/qemu: QEMU 8.2.2 bridge — runs the HolyC layer
                      ▼
hardware      GFX1100 asm · SM80 PTX · microcode disassembler
```

## Packages

| Package | What it is |
| --- | --- |
| `vm-abi/` | VM ABI substrate: ATS2 proofs, HolyC VM core, C11 substrate, tests |
| `vm-abi/qemu/` | QEMU 8.2.2 bridge: patches + bridge files that run the HolyC layer under QEMU |
| `assets/` | Diagrams and terminal screenshots for this README |
| `native-execution/` | C11 substrate: abi, value, memory, contract, kernel, dispatch, context |
| `nim-algorithm-layer/` | Nim algorithm language: lexer, parser, AST, IR, interpreter, C11 bridge |
| `occam-gpu/` | Occam frontend, GPU IR, PTX / AMDGPU / SPIR-V backends, ATS2 proofs |
| `snapkitty/` | One vertical slice: occam dispatch, B kernel, ATS2 proofs, GFX1100 asm, SM80 PTX |
| `snapkitty-heterogeneous/` | Heterogeneous runtime: device discovery, Mojo kernels, MLIR fusion passes |
| `ucode-disasm/` | C11 disassembler for undocumented 32-bit microcode |

## The Ring 0 layer — `vm-abi/`

The execution layer that admits and runs modules. Written in HolyC.

HolyC is a programming language. This layer is a VM core written in it: its
own ABI, its own admission gate, its own context and trace machinery. The
`.HC` files run under QEMU through the bridge in `vm-abi/qemu/` — there is no
native Linux build step for them. The C11 substrate builds and tests on Linux.

- **ABI v1** (`VM_ABI_VERSION 0x00010001`), module magic `VMMO`. 4K pages,
  64 regions, 32 execution contexts, 4096-entry trace ring.
- **Capabilities:** RING0, MEM, C11, JIT, TRACE, REPL.
- **Admission gate** (`VmAdmitModule`): module magic, ABI version, privilege
  level read straight from the CS register (RING0 modules require CPL 0),
  capability check, FNV-1a hash over the module header.
- **Context save/restore** (`VmCtxSave` / `VmCtxRestore`): inline asm, all 16
  general-purpose registers through the context table. New contexts start at
  ring 3.
- **C11 bridge** (`VmCallC11`): SysV calling convention — args in RDI, RSI,
  RDX, RCX, R8, R9, return in RAX.
- **REPL** (`VmReplEval`), **inspect** (`VmInspect`), **TSC-stamped trace
  ring** (`VmTraceEmit`, overwrites oldest when full), **fault recovery**
  (`VmFaultRecover`, restores saved registers).
- **Tests** (`tests/test_holyc_vm.HC`, 6): lifecycle, context create/destroy,
  admission path (hash fails on purpose — the negative path), ring (prints
  CS.CPL), trace, fault recovery.
- **Bridge** (`holyc/BridgeClient.HC`, `holyc/bridge.asm`): the HolyC side of
  the QEMU bridge — a mailbox at `0x90000`, commands `add64` / `xor_acc`.
  `holyc/Ring0Probe.HC` probes port I/O, VGA text memory, and the IDT hook.
  `holyc/Main.HC` is the entry point.

## The QEMU bridge — `vm-abi/qemu/`

Runs the HolyC layer under QEMU. `verify_bridge.sh` takes a QEMU 8.2.2
tarball and a work directory, verifies the tarball's sha256, applies the 6
patches in `patches/`, copies the 3 bridge files
(`holyc_qemu_bridge.c` / `.h`, `holyc_qemu_traps.c`) into the QEMU tree,
configures and builds `qemu-system-x86_64`, and confirms the bridge symbols
are linked into the binary. Fails closed on any warning or error.

## Build

Measured on Linux x86_64, gcc 13.3.0. Each package builds on its own.

| Package | Command | Result |
| --- | --- | --- |
| `native-execution/` | `cmake -S . -B build && cmake --build build && ctest --test-dir build` | 4/4 tests pass. `native_probe` prints `ABI v1, pointer width 64, probe=0xc011` |
| `ucode-disasm/` | `cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic ucode_disasm.c -o ucode_disasm && ./ucode_disasm` | builds, runs, 25 rows |
| `occam-gpu/` | `./build.sh` | exit 2 — `instr_select.o`, `-Werror` unused-const-variable on `ptx_rules` / `amdgpu_rules` |
| `vm-abi/` | `./build.sh` | exit 2 — `_Static_assert` in `include/vm_abi.h` |
| `nim-algorithm-layer/` | `nimble build` | exit 1 — `src/algorithm/lexer.nim:26` |
| `snapkitty-heterogeneous/` | `make` | exit 2 — cmake parse error at `CMakeLists.txt:3` |
| `snapkitty-heterogeneous/` | `mojo build mojo/kernels/*.mojo` | `module does not contain a 'main' function`, no parse/type errors |
| `snapkitty/` | `llvm-mc --triple=amdgcn-amd-amdhsa --mcpu=gfx1100 asm/vec_add_gfx1100.s` | exit 1 — 5 invalid-operand errors |
| `vm-abi/qemu/` | `bash vm-abi/qemu/verify_bridge.sh <qemu-8.2.2.tar.xz> <workdir>` | not run on this host |

Notes:

- `snapkitty-heterogeneous/CMakeLists.txt` names 9 runtime sources that are
  not in this tree: `device_context.c`, `device_capabilities.c`,
  `memory/buffer.c`, `memory/allocation.c`, `memory/transfer.c`,
  `events/event.c`, `events/synchronization.c`, `scheduler/dispatch.c`,
  `scheduler/workload.c`. The `EXISTS` guard skips them; the package builds
  from its present files only.
- No GPU execution anywhere in this tree. Kernels are assembled, never run on
  device hardware. `ptxas` / `nvcc` are not present on this host.
- `snapkitty/benchmark/benchmark.R` is outside the execution path. It is never
  part of any build.

## License

AGPLv3 only. Root `LICENSE` holds the full canonical AGPLv3 text, and every
file carries an AGPL-3.0-only header. No other license is named anywhere in
this tree.
