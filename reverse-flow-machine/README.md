# Reverse-Flow Machine

A polyglot reverse-flow virtual machine combining destination-owned control flow (C-INTERCAL), persistent transactional state (MUMPS / YottaDB), and type-level transition invariants (Haskell), with zero external dependencies and zero Haskell `import` statements.

## Architecture

| Layer | Language | Responsibility |
| :--- | :--- | :--- |
| Control Flow | C-INTERCAL (`intercal/`) | Destination-owned `COME FROM`, computed `COME FROM`, `NEXT FROM`, `ABSTAIN`, `REINSTATE`, `STASH`, `RETRIEVE`, `GIVE UP` |
| State & Dispatch | MUMPS (`mumps/`) | Persistent globals (`^STATE`, `^REVERSE`, `^ABSTAIN`, `^TRACE`), `TSTART`/`TCOMMIT`/`TROLLBACK` |
| Type Authority | Haskell (`haskell/`) | `DataKinds` + `GADTs` phase invariants, deterministic dispatch & fuel-bound verification, macro lowering |
| Grammar | EBNF (`grammar/`) | Formal syntax for reverse-flow primitives (`reverse.ebnf`) and MUMPS macro language (`macros.ebnf`) |
| FFI & Ledger | C11 (`ffi/`) | Zero-dependency C11 runtime bridge mirroring MUMPS/Haskell invariants for native verification |

## Mathematical Model

For transition relation $R = \{(s, d, g, \tau)\}$:

- Enabled destination set: $E(s, \sigma) = \{d \mid (s, d, g, \tau) \in R \land g(\sigma) = 1\}$
- Deterministic dispatch: $|E(s, \sigma)| \le 1$
- State evolution: $\sigma_{n+1} = \tau(\sigma_n)$
- Decreasing termination budget: $f(\sigma_{n+1}) < f(\sigma_n)$ with $f(\sigma) \in \mathbb{Z}_{\ge 0}$

## Building & Testing

```sh
make test
```

Optional toolchain checks (when `ghc` or `ick` are installed):

```sh
make check-haskell
make check-intercal
```
