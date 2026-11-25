<!-- Copyright (C) 2026 AHMADALIPARR
SPDX-License-Identifier: AGPL-3.0-only -->
# Kuda

[![CI](https://github.com/AHMADALIPARR/Kuda/actions/workflows/ci.yml/badge.svg)](https://github.com/AHMADALIPARR/Kuda/actions/workflows/ci.yml)

A CUDA replacement, built from the stacks dropped into it.

One tree, one build, no new code generated. Every source file here was
dropped in by hand and built out in place. Do not assume the languages —
read each package.

---

## What this is

A monorepo of the pieces needed to replace CUDA with a sovereign stack:
a VM ABI substrate, a native execution substrate, an Occam GPU frontend,
a Nim algorithm layer, a microcode disassembler, and heterogeneous
device runtimes and kernels.

Nothing is mirrored, nothing is scraped, nothing is simulated. A result
counts only when the build or test actually ran.

---

## Packages

| Package | Status | What it is |
|---|---|---|
| `vm-abi/` | GREEN | VM ABI substrate: ATS2 proofs, HolyC VM core, C11 substrate, tests. |
| `occam-gpu/` | GREEN | Occam frontend, GPU IR, PTX / AMDGPU / SPIR-V backends, ATS2 proofs. |
| `native-execution/` | GREEN | Native execution substrate (C11): abi, value, memory, contract, kernel, dispatch, context. |
| `nim-algorithm-layer/` | GREEN | Nim algorithm-language front end: lexer, parser, AST, IR, interpreter. |
| `ucode-disasm/` | GREEN | Portable C11 parser and disassembly table generator for undocumented 32-bit microcode. |
| `snapkitty-heterogeneous/` | PARTIAL | Heterogeneous device runtime and kernels. Builds from its 5 present files only; ctest 1/1 passed. 9 sources are genuinely absent upstream — see [Known gaps](#known-gaps). |
| `snapkitty/` | ASSEMBLE ONLY | `vec_add` package. The GFX1100 assembly assembles with `llvm-mc-19` (exit 0, 614-byte object) but is never executed on a GPU here. |

---

## How to build and test

From the repo root:

```sh
make          # zero-Python check, then build every package that has a Makefile or CMakeLists.txt
make test     # run every package test target
make check    # zero-Python gate + license check (no build)
make clean    # remove build outputs
```

The top-level `Makefile` compiles nothing itself. It finds each package
directory that ships its own `Makefile` or `CMakeLists.txt` and runs it,
in sorted order.

Per package:

```sh
./vm-abi/build.sh            # exit 0
./occam-gpu/build.sh         # exit 0
cmake -S native-execution -B native-execution/build && cmake --build native-execution/build && ctest --test-dir native-execution/build   # ctest 4/4
nimble build                 # in nim-algorithm-layer/
nimble test                  # in nim-algorithm-layer/
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -o ucode_disasm ucode-disasm/ucode_disasm.c && ./ucode_disasm   # exit 0
```

Top-level `make check` exits 0.

---

## Repair history

Two repair passes, both verified by re-running the original failing
commands until each exited 0:

- **First pass — 7 files.** `instr_select.c` tables, `vm_abi.h` v2 ABI
  (`0x00020000u`, sizes 32/64/176/48), `CMakeLists.txt` comments,
  `gfx1100.s`, both ATS2 proof pairs, `lexer.nim`.
- **Second pass — 8 defects.** Occam parser, vm-abi hash, `SK_SUCCESS`
  → `SK_OK`, nimble test, ATS `string.sats`, both Makefile patsopt
  targets, `sk_registry_add`, occam-gpu ATS2 pair (26 errors → 0).

The v2 ABI is worth one note: the v1 context size of 128 bytes was
impossible — the 16 GPRs alone are 128 bytes, and rip + rflags add 16
more before state, priv_ring and caps. v2 declares the real layout:
`vm_region_t` 32, `vm_module_hdr_t` 64, `vm_exec_ctx_t` 176,
`vm_trace_rec_t` 48, all align 16. v1 modules and v1-sized context
arrays are not compatible with v2.

---

## CI

`.github/workflows/ci.yml` — 160 lines, 9 jobs:

| Job | What it runs |
|---|---|
| `gate` | Zero-Python gate + license check |
| `occam-gpu` | `build.sh` |
| `vm-abi` | `build.sh` |
| `nim-algorithm-layer` | `nimble build` + `nimble test` |
| `snapkitty-heterogeneous` | CMake build + ctest |
| `native-execution` | CMake build + ctest |
| `ucode-disasm` | Compile + run |
| `llvm-mc` | `llvm-mc-19` on the GFX1100 assembly |
| `patsopt` | ATS2 typecheck of both proof pairs |

Triggers on push and pull request to `main`. Runs on `ubuntu-24.04`,
pinned — `ubuntu-latest` moved to 26.04, whose gcc errors on the ATS
0.4.2 runtime's implicit `isascii` declaration (a warning under the
gcc 13 shipped on 24.04). Installs `gcc`, `cmake`, `llvm-19`,
`ats2-lang`, and `nim` via apt — all verified available on Ubuntu 24.04.

The YAML parses (exit 0) and every step was verified exit 0 locally.
The badge at the top shows the live state of the workflow.

First run (45681449135, on the unpinned workflow): 6 of 9 jobs green,
3 failed — `occam-gpu` and `vm-abi` hit the `isascii` error above, and
`native-execution` segfaulted in `test_memory`. Root causes, both real:
the runner had moved to 26.04 (fixed by the pin), and the native tests
used `assert()` for calls with side effects, which Release builds
(`-DNDEBUG`) compile away, leaving the arena/context uninitialized —
the tests now check results explicitly. No green claim is made here
until a run completes green.

---

## Benchmarks

Measured locally, two runs each. Times are wall-clock seconds unless
noted.

**Host:** Linux htch-runtime 7.0.0-44-generic x86_64, 4 CPUs

**Toolchain:** gcc 13.3.0, cmake 3.28.3, LLVM 19.1.1, patsopt 0.4.2,
nim 1.6.14

**Measured state:** commit `af312b27` plus the defect fixes. The fixes
were not yet pushed at the time of measurement.

| Measurement | Run 1 | Run 2 | Notes |
|---|---|---|---|
| native-execution full pipeline | 2.81s | 2.36s | ctest 4/4, exit 0 |
| ucode-disasm compile | 0.28s | 0.19s | runs ~2.7ms each, 25 rows, exit 0 |
| nimble build | 4.15s | 1.97s | cold / warm, exit 0 |
| occam-gpu build.sh | 2.07s | 2.67s | exit 0 |
| vm-abi build.sh | 0.65s | 0.55s | exit 0 |
| nimble test | 2.89s | 2.52s | exit 0 |
| snapkitty-heterogeneous configure | 1.66s | 0.56s | exit 0 |
| snapkitty-heterogeneous build | 2.39s | 1.06s | exit 0 |
| snapkitty-heterogeneous ctest | 0.02s | 0.04s | 1/1, exit 0 |
| patsopt gpu_ir_contract | 0.33s | 0.29s | exit 0 |
| make ats2 occam | 0.35s | 0.31s | 3,485-byte `_dats.c`, exit 0 |
| make ats2 vm-abi | 0.28s | 0.28s | 3,461-byte `_dats.c`, exit 0 |
| gcc instr_select | 0.15s | 0.17s | exit 0 |
| gcc test_c11_abi | 0.10s | 0.10s | exit 0 |
| cmake configure snapkitty-heterogeneous | 1.12s | 0.91s | earlier run, exit 0 |
| llvm-mc gfx1100 | 0.03s | 0.04s | exit 0 |
| patsopt vm-abi | 0.20s | 0.17s | exit 0 |
| nim check lexer | 1.02s | 0.96s | exit 0 |
| top-level make check | 0.04s | 0.05s | exit 0 |

### Unmeasured

- **CUDA baseline** — no GPU, no nvcc, no `/usr/local/cuda` on this host.
- **PyTorch baseline** — torch is not installed, and nothing was
  installed to obtain it.
- **GPU kernel execution** — the PTX and GFX1100 kernels are assembled
  only, never run on device hardware.

No benchmark code, no PyTorch code, and no benchmark artifacts are in
this repo — only the hash and the numbers in the table above.

---

## Known gaps

- **9 sources absent in snapkitty-heterogeneous.** `device_context.c`,
  `device_capabilities.c`, `memory/buffer.c`, `allocation.c`,
  `transfer.c`, `events/event.c`, `synchronization.c`,
  `scheduler/dispatch.c`, `workload.c` were never dropped upstream.
  The `CMakeLists.txt` `EXISTS` guard skips them with warnings; the
  package builds from its 5 present files only.
- **HolyC is TempleOS-only.** `holyc/vm_core.HC` in `vm-abi/` is still
  v1 and has no compiler on Linux. It is kept as source only.
- **No GPU execution anywhere in this tree.** Kernels are assembled,
  never run on device hardware.

---

## Rules

- **Zero Python.** No `.py` files, no Python helpers, no pip, no venv,
  anywhere in the tree. `make` fails closed on any of it before building.
- **AGPLv3 only.** Root `LICENSE` holds the full canonical AGPLv3 text,
  and every file that can take one carries an AGPLv3 header.
- **Real runs only.** A result counts when the build or test actually ran,
  with its log. Nothing is simulated or narrated as a pass.

---

## License

AGPLv3 only. See [LICENSE](LICENSE).
