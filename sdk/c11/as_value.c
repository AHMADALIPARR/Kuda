// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* as_value.c - value model: construction, casts, type helpers.
 * Numeric conversions follow C rules; bool is 0/1.
 */

#include <string.h>
#include "as_c11.h"

as_value as_value_void(void) { as_value v; memset(&v, 0, sizeof v); v.kind = AS_T_VOID; return v; }
as_value as_value_bool(int b) { as_value v; memset(&v, 0, sizeof v); v.kind = AS_T_BOOL; v.v.b = b ? 1 : 0; return v; }
as_value as_value_int(int32_t x) { as_value v; memset(&v, 0, sizeof v); v.kind = AS_T_INT; v.v.i32 = x; return v; }
as_value as_value_int64(int64_t x) { as_value v; memset(&v, 0, sizeof v); v.kind = AS_T_INT64; v.v.i64 = x; return v; }
as_value as_value_uint(uint32_t x) { as_value v; memset(&v, 0, sizeof v); v.kind = AS_T_UINT; v.v.u32 = x; return v; }
as_value as_value_uint64(uint64_t x) { as_value v; memset(&v, 0, sizeof v); v.kind = AS_T_UINT64; v.v.u64 = x; return v; }
as_value as_value_float(float x) { as_value v; memset(&v, 0, sizeof v); v.kind = AS_T_FLOAT; v.v.f32 = x; return v; }
as_value as_value_double(double x) { as_value v; memset(&v, 0, sizeof v); v.kind = AS_T_DOUBLE; v.v.f64 = x; return v; }

int as_type_is_integer(as_type t)
{
    return t == AS_T_BOOL || t == AS_T_INT || t == AS_T_INT64 ||
           t == AS_T_UINT || t == AS_T_UINT64;
}

int as_type_is_float(as_type t)
{
    return t == AS_T_FLOAT || t == AS_T_DOUBLE;
}

int as_type_is_numeric(as_type t)
{
    return as_type_is_integer(t) || as_type_is_float(t);
}

const char *as_type_name(as_type t)
{
    switch (t) {
    case AS_T_VOID: return "void";
    case AS_T_BOOL: return "bool";
    case AS_T_INT: return "int";
    case AS_T_INT64: return "int64";
    case AS_T_UINT: return "uint";
    case AS_T_UINT64: return "uint64";
    case AS_T_FLOAT: return "float";
    case AS_T_DOUBLE: return "double";
    default: return "?";
    }
}

int as_type_from_keyword(const char *kw)
{
    if (strcmp(kw, "void") == 0) return AS_T_VOID;
    if (strcmp(kw, "bool") == 0) return AS_T_BOOL;
    if (strcmp(kw, "int") == 0) return AS_T_INT;
    if (strcmp(kw, "int8") == 0) return AS_T_INT;   /* alias */
    if (strcmp(kw, "int16") == 0) return AS_T_INT;  /* alias */
    if (strcmp(kw, "int64") == 0) return AS_T_INT64;
    if (strcmp(kw, "uint") == 0) return AS_T_UINT;
    if (strcmp(kw, "uint8") == 0) return AS_T_UINT;  /* alias */
    if (strcmp(kw, "uint16") == 0) return AS_T_UINT; /* alias */
    if (strcmp(kw, "uint32") == 0) return AS_T_UINT; /* alias */
    if (strcmp(kw, "uint64") == 0) return AS_T_UINT64;
    if (strcmp(kw, "float") == 0) return AS_T_FLOAT;
    if (strcmp(kw, "double") == 0) return AS_T_DOUBLE;
    return -1;
}

int as_value_truthy(const as_value *v)
{
    switch (v->kind) {
    case AS_T_BOOL: return v->v.b != 0;
    case AS_T_INT: return v->v.i32 != 0;
    case AS_T_INT64: return v->v.i64 != 0;
    case AS_T_UINT: return v->v.u32 != 0;
    case AS_T_UINT64: return v->v.u64 != 0;
    case AS_T_FLOAT: return v->v.f32 != 0.0f;
    case AS_T_DOUBLE: return v->v.f64 != 0.0;
    default: return 0;
    }
}

/* Common arithmetic type (C usual arithmetic conversions, simplified). */
as_type as_common_type(as_type a, as_type b)
{
    if (a == AS_T_DOUBLE || b == AS_T_DOUBLE) return AS_T_DOUBLE;
    if (a == AS_T_FLOAT || b == AS_T_FLOAT) return AS_T_FLOAT;
    if (a == AS_T_UINT64 || b == AS_T_UINT64) return AS_T_UINT64;
    if (a == AS_T_INT64 || b == AS_T_INT64) return AS_T_INT64;
    if (a == AS_T_UINT || b == AS_T_UINT) return AS_T_UINT;
    return AS_T_INT;
}

int as_value_cast(const as_value *in, as_type to, as_value *out)
{
    if (to == AS_T_VOID) { *out = as_value_void(); return 0; }
    if (in->kind == to) { *out = *in; return 0; }
    if (to == AS_T_BOOL) { out->kind = AS_T_BOOL; out->v.b = as_value_truthy(in); return 0; }
    if (!as_type_is_numeric(in->kind) || !as_type_is_numeric(to)) return -1;
    switch (to) {
    case AS_T_INT:
        out->kind = AS_T_INT;
        switch (in->kind) {
        case AS_T_BOOL: out->v.i32 = in->v.b; break;
        case AS_T_INT: out->v.i32 = in->v.i32; break;
        case AS_T_INT64: out->v.i32 = (int32_t)in->v.i64; break;
        case AS_T_UINT: out->v.i32 = (int32_t)in->v.u32; break;
        case AS_T_UINT64: out->v.i32 = (int32_t)in->v.u64; break;
        case AS_T_FLOAT: out->v.i32 = (int32_t)in->v.f32; break;
        case AS_T_DOUBLE: out->v.i32 = (int32_t)in->v.f64; break;
        default: return -1;
        }
        return 0;
    case AS_T_INT64:
        out->kind = AS_T_INT64;
        switch (in->kind) {
        case AS_T_BOOL: out->v.i64 = in->v.b; break;
        case AS_T_INT: out->v.i64 = in->v.i32; break;
        case AS_T_INT64: out->v.i64 = in->v.i64; break;
        case AS_T_UINT: out->v.i64 = in->v.u32; break;
        case AS_T_UINT64: out->v.i64 = (int64_t)in->v.u64; break;
        case AS_T_FLOAT: out->v.i64 = (int64_t)in->v.f32; break;
        case AS_T_DOUBLE: out->v.i64 = (int64_t)in->v.f64; break;
        default: return -1;
        }
        return 0;
    case AS_T_UINT:
        out->kind = AS_T_UINT;
        switch (in->kind) {
        case AS_T_BOOL: out->v.u32 = (uint32_t)in->v.b; break;
        case AS_T_INT: out->v.u32 = (uint32_t)in->v.i32; break;
        case AS_T_INT64: out->v.u32 = (uint32_t)in->v.i64; break;
        case AS_T_UINT: out->v.u32 = in->v.u32; break;
        case AS_T_UINT64: out->v.u32 = (uint32_t)in->v.u64; break;
        case AS_T_FLOAT: out->v.u32 = (uint32_t)in->v.f32; break;
        case AS_T_DOUBLE: out->v.u32 = (uint32_t)in->v.f64; break;
        default: return -1;
        }
        return 0;
    case AS_T_UINT64:
        out->kind = AS_T_UINT64;
        switch (in->kind) {
        case AS_T_BOOL: out->v.u64 = (uint64_t)in->v.b; break;
        case AS_T_INT: out->v.u64 = (uint64_t)in->v.i32; break;
        case AS_T_INT64: out->v.u64 = (uint64_t)in->v.i64; break;
        case AS_T_UINT: out->v.u64 = in->v.u32; break;
        case AS_T_UINT64: out->v.u64 = in->v.u64; break;
        case AS_T_FLOAT: out->v.u64 = (uint64_t)in->v.f32; break;
        case AS_T_DOUBLE: out->v.u64 = (uint64_t)in->v.f64; break;
        default: return -1;
        }
        return 0;
    case AS_T_FLOAT:
        out->kind = AS_T_FLOAT;
        switch (in->kind) {
        case AS_T_BOOL: out->v.f32 = (float)in->v.b; break;
        case AS_T_INT: out->v.f32 = (float)in->v.i32; break;
        case AS_T_INT64: out->v.f32 = (float)in->v.i64; break;
        case AS_T_UINT: out->v.f32 = (float)in->v.u32; break;
        case AS_T_UINT64: out->v.f32 = (float)in->v.u64; break;
        case AS_T_FLOAT: out->v.f32 = in->v.f32; break;
        case AS_T_DOUBLE: out->v.f32 = (float)in->v.f64; break;
        default: return -1;
        }
        return 0;
    case AS_T_DOUBLE:
        out->kind = AS_T_DOUBLE;
        switch (in->kind) {
        case AS_T_BOOL: out->v.f64 = (double)in->v.b; break;
        case AS_T_INT: out->v.f64 = (double)in->v.i32; break;
        case AS_T_INT64: out->v.f64 = (double)in->v.i64; break;
        case AS_T_UINT: out->v.f64 = (double)in->v.u32; break;
        case AS_T_UINT64: out->v.f64 = (double)in->v.u64; break;
        case AS_T_FLOAT: out->v.f64 = (double)in->v.f32; break;
        case AS_T_DOUBLE: out->v.f64 = in->v.f64; break;
        default: return -1;
        }
        return 0;
    default:
        return -1;
    }
}
