<!-- Copyright (C) 2026 AHMADALIPARR
SPDX-License-Identifier: AGPL-3.0-only -->
# Native Execution Substrate (C11)

A small C11 foundation intended to sit below a future HolyC execution VM and
beside an ATS2 static-verification layer.

## Responsibilities

- `abi`: versioned host/VM boundary descriptor.
- `value`: fixed tagged value representation for the initial foreign-call ABI.
- `memory`: bounded arena with marks and rewind.
- `contract`: runtime admission checks for capability and byte-count claims.
- `kernel`: bounded kernel registry.
- `dispatch`: validates a registered kernel's contract before invoking it.
- `context`: owns the arena, registry, capability set, and dispatch counter.

The C11 layer does not implement a bytecode interpreter or claim to prove
ATS2 properties. Runtime contracts are defense-in-depth checks. ATS2 proof
artifacts and HolyC VM admission are a subsequent integration.

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/native_probe
```

## ABI rules

1. Kernel IDs are nonzero and unique within a registry.
2. A kernel descriptor and its `name` storage must outlive the registry.
3. `sk_value_ptr` is non-owning. Ownership must be specified by the caller's
   ATS2 contract or a later VM-side handle table.
4. The arena is context-owned and is not thread-safe.
5. A successful dispatch increments `dispatch_count`; failed dispatches do not.
6. The current ABI is process-local. It is not a stable wire or disk format.

## Current limitations

This is a complete minimal substrate, not a production kernel or security
boundary. It intentionally omits dynamic library loading, executable-memory
management, threads, device drivers, interrupt handling, and a verified
HolyC/C ABI bridge. Those require target-specific design and tests.
