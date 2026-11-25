<!-- Copyright (C) 2026 AHMADALIPARR
SPDX-License-Identifier: AGPL-3.0-only -->
# MONOREPO

A CUDA replacement, built from the stacks dropped into it. One tree, one
build, no new code generated — every source file in this tree is dropped in
by hand and built out here. Do not assume the languages: read each package.

## Packages

| Path | What lives there |
|---|---|
| `vm-abi/` | VM ABI substrate: ATS2 proofs (`proofs/`), HolyC VM core (`holyc/`), C11 substrate (`src/`, `include/`), tests. Make + `build.sh`. |
| `native-execution/` | Native execution substrate (C11): abi, value, memory, contract, kernel, dispatch, context. CMake + ctest. Sits below the HolyC VM, beside the ATS2 layer. |
| `tools/` | Build and verification scripts (Bash only) |
| `docs/` | Notes, specs and run records |

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
  verbatim under the no-edit rule) — 0 of 32 files headed. A push would
  fail the license check until headers are added or Ahmad says otherwise.
- **Real runs only.** A result counts when the build or test actually ran,
  with its log. Nothing is simulated or narrated as a pass.

## License

AGPLv3 only. See [LICENSE](LICENSE).

## vm-abi ABI v2 (2026-11-25)

`vm-abi/include/vm_abi.h` repaired to ABI v2, `VM_ABI_VERSION 0x00020000u`.
v1 claimed sizes 32/64/128/48, align >= 16; measured v1 was 32/56/168/40,
align 8 (7 of 8 static asserts failed). v2 declares the real layout:
`vm_region_t` 32, `vm_module_hdr_t` 64, `vm_exec_ctx_t` **176**,
`vm_trace_rec_t` 48, all align 16, with every field offset pinned by
`_Static_assert(offsetof(...))` in the header. No field was deleted.

The 128-byte v1 context is impossible: the 16 GPRs alone are 128 bytes;
rip + rflags add 16 more (144) before state, priv_ring and caps. Full
arithmetic: 128 + 8 + 8 + 4 + 4 + 8 + 8 = 168, rounded up to a multiple
of 16 = 176. v1 modules / v1-sized context arrays are not compatible
with v2. Still to sync: `holyc/vm_core.HC` version define (still
0x00010001), ATS2 proofs + `tests/test_ats2_proofs.dats` (still encode
ctx size 128 — must become 176).

Remaining, outside the header: `gcc -c tests/test_c11_abi.c` exits 0 and
the sizes probe prints 32/64/176/48, align 16 — but the linked test
binary still aborts at `vm_verify_module` (`VM_ERR_HASH`, -5), because
`src/c11_substrate.c` hashes a range that includes the `abi_hash` and
`proof_hash` fields themselves, which are zero when the test computes
`abi_hash` and set by verify time. That gate lives in the substrate,
not the header, and no header reordering can fix it under the test's
compute order.
