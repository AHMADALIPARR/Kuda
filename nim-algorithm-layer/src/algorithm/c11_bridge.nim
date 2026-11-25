# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
## Narrow C11 boundary. This module intentionally does not duplicate the
## C11 kernel registry or memory ownership. Compile/link against sk_native.
{.passC: "-I../native-execution/include".}
{.passL: "-L../native-execution/build -lsk_native".}

type
  SkStatus* {.size: sizeof(cint).} = enum
    skOk = 0, skErrInvalidArgument, skErrOutOfMemory, skErrOutOfBounds,
    skErrTypeMismatch, skErrDuplicate, skErrNotFound, skErrContract,
    skErrCapacity, skErrInternal
  SkValueKind* {.size: sizeof(cint).} = enum
    skValueUnit = 0, skValueI64, skValueU64, skValueF64, skValueBool, skValuePtr
  SkValue* {.importc: "sk_value", header: "sk/value.h", bycopy.} = object
    kind*: SkValueKind
    payload*: array[8, byte]

proc skValueI64*(value: int64): SkValue {.importc: "sk_value_i64", cdecl, header: "sk/value.h".}
proc skValueAsI64*(value: SkValue, output: ptr int64): SkStatus
  {.importc: "sk_value_as_i64", cdecl, header: "sk/value.h".}
proc skStatusString*(status: SkStatus): cstring
  {.importc: "sk_status_string", cdecl, header: "sk/status.h".}
