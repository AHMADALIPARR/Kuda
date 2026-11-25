<!-- Copyright (C) 2026 AHMADALIPARR
SPDX-License-Identifier: AGPL-3.0-only -->
# Kuda

A CUDA replacement monorepo, built from the stacks dropped into it. One tree, one
build, no new code generated — every source file in this tree is dropped in
by hand and built out here. Do not assume the languages: read each package.

## Packages

| Path | What lives there |
|---|---|
| `vm-abi/` | VM ABI substrate: ATS2 proofs (`proofs/`), HolyC VM core (`holyc/`), C11 substrate (`src/`, `include/`), tests. Make + `build.sh`. |
| `native-execution/` | Native execution substrate (C11): abi, value, memory, contract, kernel, dispatch, context. CMake + ctest. Sits below the HolyC VM, beside the ATS2 layer. |
| `nim-algorithm-layer/` | Nim front-end and interpreter for the algorithm language. Stack-oriented IR, integer arithmetic, overflow detection. |
| `occam-gpu/` | Occam GPU IR: frontend, instruction selection, backends for PTX / AMDGPU / SPIR-V, ATS2 contracts. |
| `snapkitty/` | SnapKitty kernels and proofs: Occam dispatch, B, assembly (PTX/GFX), ATS2 verification, R benchmark. |
| `snapkitty-heterogeneous/` | Heterogeneous SnapKitty: MLIR fusion passes, Mojo kernels, C runtime device discovery (NVIDIA/AMD/Linux), CMake + Make. |
| `ucode-disasm/` | Microcode disassembler (C). |
| `tools/` | Build and verification scripts (Bash only). |
| `docs/` | Notes, specs and run records. |

Drops land in `~/workspace/user/files/` first, then get extracted into the
matching package directory. Nothing is generated, nothing is mirrored,
nothing is scraped. A dropped file keeps its bytes — **no edits at all**,
not even license headers, unless Ahmad says so.

## Build

```sh
make          # zero-Python check, then build every package that has a Makefile or CMakeLists.txt
make test     # run every package test target
make check    # zero-Python gate + license check (no build)
make clean    # remove build outputs
```

The top-level `Makefile` does not compile anything itself. It finds each
package directory that ships its own `Makefile` or `CMakeLists.txt` and
runs it, in sorted order. A package that has no build file is listed,
not built.

## Build status (2026-11-25, real runs on this host)

- `native-execution`: **green**. CMake build, ctest 4/4 passed (abi, memory,
  kernel, dispatch), `native_probe` prints `ABI v1, pointer width 64,
  probe=0xc011`.
- `vm-abi`: **red, defects in the dropped code, not fixed** (no-edit rule).
  `include/vm_abi.h` static asserts fail: actual sizes are 32/56/168/40
  bytes vs claimed 32/64/128/48, alignment 8 vs claimed >= 16.
  The ATS2 proofs do not typecheck under patsopt 0.4.2 (sort errors,
  unrecognized `land`, `implement` used where `primplement` is required),
  and the Makefile's `patsopt` invocation is ignored by this version
  (bare `.dats` argument), producing a 0-byte `abi_contract_dats.c`.
  The HolyC files have no compiler on Linux (TempleOS only) and are kept
  as source only.

## Rules

- **Zero Python.** No `.py` files, no Python helpers, no pip, no venv,
  anywhere in the tree. `make` fails closed on any of it before building.
- **AGPLv3 only.** Root `LICENSE` holds the full canonical AGPLv3 text.
  Dropped files currently carry **no** AGPLv3 headers (they were placed
  verbatim under the no-edit rule). The Makefile `check` target verifies
  the root LICENSE is AGPLv3.
- **Real runs only.** A result counts when the build or test actually ran,
  with its log. Nothing is simulated or narrated as a pass.

## License

AGPLv3 only. See [LICENSE](LICENSE).
