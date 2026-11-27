// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* as_interp.c - tree-walking interpreter + compile-time validator.
 * Scopes, expressions, statements, function calls, execution bounds.
 * Registered C functions are invoked through the x86-64 trampoline
 * (as_engine_call_registered in as_engine.c).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "as_c11.h"

/* ---------------- scopes ---------------- */

typedef struct as_var {
    char *name;
    as_type type;
    as_value val;
} as_var;

typedef struct as_scope {
    struct as_scope *parent;
    as_var *vars;
    int nvars, capvars;
} as_scope;

static as_scope *scope_new(as_scope *parent)
{
    as_scope *s = as_xmalloc(sizeof *s);
    memset(s, 0, sizeof *s);
    s->parent = parent;
    return s;
}

static void scope_free(as_scope *s)
{
    int i;
    for (i = 0; i < s->nvars; i++) free(s->vars[i].name);
    free(s->vars);
    free(s);
}

static as_var *scope_define(as_scope *s, const char *name, as_type type)
{
    int i;
    as_var *v;
    for (i = 0; i < s->nvars; i++) {
        if (strcmp(s->vars[i].name, name) == 0) return NULL; /* redeclare */
    }
    if (s->nvars >= s->capvars) {
        s->capvars = s->capvars ? s->capvars * 2 : 8;
        s->vars = as_xrealloc(s->vars, (size_t)s->capvars * sizeof *s->vars);
    }
    v = &s->vars[s->nvars++];
    memset(v, 0, sizeof *v);
    v->name = as_xstrdup(name);
    v->type = type;
    v->val = as_value_void();
    /* default-initialize numerics to zero */
    if (as_type_is_numeric(type)) as_value_cast(&(as_value){ .kind = AS_T_INT, .v.i32 = 0 }, type, &v->val);
    else if (type == AS_T_BOOL) v->val = as_value_bool(0);
    return v;
}

static as_var *scope_lookup(as_scope *s, const char *name)
{
    int i;
    for (; s; s = s->parent) {
        for (i = 0; i < s->nvars; i++) {
            if (strcmp(s->vars[i].name, name) == 0) return &s->vars[i];
        }
    }
    return NULL;
}

/* ---------------- exceptions / bounds ---------------- */

static int set_exception(as_exec *ex, int line, const char *msg)
{
    asIScriptContext *ctx = ex->ctx;
    snprintf(ctx->exception, sizeof ctx->exception, "%s", msg);
    ctx->exception_line = line;
    ctx->exception_col = 0;
    snprintf(ctx->exception_section, sizeof ctx->exception_section, "%s",
             ex->module ? ex->module->name : "");
    return -1;
}

static int check_bounds(as_exec *ex, int line)
{
    if (ex->ctx->abort_flag) {
        ex->aborted = 1;
        return set_exception(ex, line, "execution aborted");
    }
    /* Fuel is the reverse-flow-machine step budget (ctx->rf.fuel). Each
     * interpreter step burns one unit; exhaustion is an exception.
     * One unit is always reserved for the lifecycle transition
     * (VALIDATED -> COMMITTED/REJECTED), so the step fails when the
     * remaining fuel is at the reserve instead of draining it to zero
     * (otherwise rf_step could not record the rejection). The machine
     * is seeded with max_steps + lifecycle headroom in Prepare. */
    if (ex->ctx->rf.fuel <= 1)
        return set_exception(ex, line, "fuel exhausted (step limit exceeded)");
    --ex->ctx->rf.fuel;
    return 0;
}

/* ---------------- binary / unary ops ---------------- */

/* Evaluate a binary operator on two already-evaluated values. */
static int eval_binop(as_exec *ex, int line, int op,
                      const as_value *l, const as_value *r, as_value *out)
{
    as_type ct;
    as_value a, b;
    if (op == TOK_AND || op == TOK_OR) {
        /* handled by the caller (short-circuit); should not reach here */
        return set_exception(ex, line, "internal error: logical op");
    }
    if (op == TOK_EQ || op == TOK_NE || op == TOK_LT || op == TOK_LE ||
        op == TOK_GT || op == TOK_GE) {
        ct = as_common_type(l->kind, r->kind);
        if (as_value_cast(l, ct, &a) || as_value_cast(r, ct, &b))
            return set_exception(ex, line, "invalid operands for comparison");
        int cmp = 0;
        switch (ct) {
        case AS_T_DOUBLE: cmp = (a.v.f64 > b.v.f64) - (a.v.f64 < b.v.f64); break;
        case AS_T_FLOAT: cmp = (a.v.f32 > b.v.f32) - (a.v.f32 < b.v.f32); break;
        case AS_T_INT64: cmp = (a.v.i64 > b.v.i64) - (a.v.i64 < b.v.i64); break;
        case AS_T_UINT64: cmp = (a.v.u64 > b.v.u64) - (a.v.u64 < b.v.u64); break;
        case AS_T_UINT: cmp = (a.v.u32 > b.v.u32) - (a.v.u32 < b.v.u32); break;
        default: cmp = (a.v.i32 > b.v.i32) - (a.v.i32 < b.v.i32); break;
        }
        int res = 0;
        switch (op) {
        case TOK_EQ: res = cmp == 0; break;
        case TOK_NE: res = cmp != 0; break;
        case TOK_LT: res = cmp < 0; break;
        case TOK_LE: res = cmp <= 0; break;
        case TOK_GT: res = cmp > 0; break;
        case TOK_GE: res = cmp >= 0; break;
        default: break;
        }
        *out = as_value_bool(res);
        return 0;
    }
    if (op == TOK_BITAND || op == TOK_BITOR || op == TOK_BITXOR ||
        op == TOK_SHL || op == TOK_SHR) {
        if (!as_type_is_integer(l->kind) || !as_type_is_integer(r->kind))
            return set_exception(ex, line, "bitwise/shift operators require integer operands");
        ct = as_common_type(l->kind, r->kind);
        if (as_value_cast(l, ct, &a) || as_value_cast(r, ct, &b))
            return set_exception(ex, line, "invalid operands for bitwise operator");
        switch (ct) {
        case AS_T_INT64: {
            int64_t x = a.v.i64, y = b.v.i64, z = 0;
            switch (op) {
            case TOK_BITAND: z = x & y; break;
            case TOK_BITOR: z = x | y; break;
            case TOK_BITXOR: z = x ^ y; break;
            case TOK_SHL: z = (int64_t)((uint64_t)x << (y & 63)); break;
            case TOK_SHR: z = x >> (y & 63); break;
            default: break;
            }
            *out = as_value_int64(z);
            return 0;
        }
        case AS_T_UINT64: {
            uint64_t x = a.v.u64, y = b.v.u64, z = 0;
            switch (op) {
            case TOK_BITAND: z = x & y; break;
            case TOK_BITOR: z = x | y; break;
            case TOK_BITXOR: z = x ^ y; break;
            case TOK_SHL: z = x << (y & 63); break;
            case TOK_SHR: z = x >> (y & 63); break;
            default: break;
            }
            *out = as_value_uint64(z);
            return 0;
        }
        case AS_T_UINT: {
            uint32_t x = a.v.u32, y = b.v.u32, z = 0;
            switch (op) {
            case TOK_BITAND: z = x & y; break;
            case TOK_BITOR: z = x | y; break;
            case TOK_BITXOR: z = x ^ y; break;
            case TOK_SHL: z = x << (y & 31); break;
            case TOK_SHR: z = x >> (y & 31); break;
            default: break;
            }
            *out = as_value_uint(z);
            return 0;
        }
        default: {
            int32_t x = a.v.i32, y = b.v.i32, z = 0;
            switch (op) {
            case TOK_BITAND: z = x & y; break;
            case TOK_BITOR: z = x | y; break;
            case TOK_BITXOR: z = x ^ y; break;
            case TOK_SHL: z = (int32_t)((uint32_t)x << (y & 31)); break;
            case TOK_SHR: z = x >> (y & 31); break;
            default: break;
            }
            *out = as_value_int(z);
            return 0;
        }
        }
    }
    /* arithmetic: + - * / % */
    if (!as_type_is_numeric(l->kind) || !as_type_is_numeric(r->kind))
        return set_exception(ex, line, "arithmetic requires numeric operands");
    ct = as_common_type(l->kind, r->kind);
    if (as_value_cast(l, ct, &a) || as_value_cast(r, ct, &b))
        return set_exception(ex, line, "invalid operands for arithmetic");
    switch (ct) {
    case AS_T_DOUBLE: {
        double x = a.v.f64, y = b.v.f64, z = 0;
        switch (op) {
        case TOK_PLUS: z = x + y; break;
        case TOK_MINUS: z = x - y; break;
        case TOK_STAR: z = x * y; break;
        case TOK_SLASH: z = x / y; break;
        case TOK_PERCENT: return set_exception(ex, line, "'%' is not defined for floating-point operands");
        default: return set_exception(ex, line, "unknown arithmetic operator");
        }
        *out = as_value_double(z);
        return 0;
    }
    case AS_T_FLOAT: {
        float x = a.v.f32, y = b.v.f32, z = 0;
        switch (op) {
        case TOK_PLUS: z = x + y; break;
        case TOK_MINUS: z = x - y; break;
        case TOK_STAR: z = x * y; break;
        case TOK_SLASH: z = x / y; break;
        case TOK_PERCENT: return set_exception(ex, line, "'%' is not defined for floating-point operands");
        default: return set_exception(ex, line, "unknown arithmetic operator");
        }
        *out = as_value_float(z);
        return 0;
    }
    case AS_T_INT64: {
        int64_t x = a.v.i64, y = b.v.i64, z = 0;
        if ((op == TOK_SLASH || op == TOK_PERCENT) && y == 0)
            return set_exception(ex, line, "division by zero");
        switch (op) {
        case TOK_PLUS: z = x + y; break;
        case TOK_MINUS: z = x - y; break;
        case TOK_STAR: z = x * y; break;
        case TOK_SLASH: z = x / y; break;
        case TOK_PERCENT: z = x % y; break;
        default: return set_exception(ex, line, "unknown arithmetic operator");
        }
        *out = as_value_int64(z);
        return 0;
    }
    case AS_T_UINT64: {
        uint64_t x = a.v.u64, y = b.v.u64, z = 0;
        if ((op == TOK_SLASH || op == TOK_PERCENT) && y == 0)
            return set_exception(ex, line, "division by zero");
        switch (op) {
        case TOK_PLUS: z = x + y; break;
        case TOK_MINUS: z = x - y; break;
        case TOK_STAR: z = x * y; break;
        case TOK_SLASH: z = x / y; break;
        case TOK_PERCENT: z = x % y; break;
        default: return set_exception(ex, line, "unknown arithmetic operator");
        }
        *out = as_value_uint64(z);
        return 0;
    }
    case AS_T_UINT: {
        uint32_t x = a.v.u32, y = b.v.u32, z = 0;
        if ((op == TOK_SLASH || op == TOK_PERCENT) && y == 0)
            return set_exception(ex, line, "division by zero");
        switch (op) {
        case TOK_PLUS: z = x + y; break;
        case TOK_MINUS: z = x - y; break;
        case TOK_STAR: z = x * y; break;
        case TOK_SLASH: z = x / y; break;
        case TOK_PERCENT: z = x % y; break;
        default: return set_exception(ex, line, "unknown arithmetic operator");
        }
        *out = as_value_uint(z);
        return 0;
    }
    default: {
        int32_t x = a.v.i32, y = b.v.i32, z = 0;
        if ((op == TOK_SLASH || op == TOK_PERCENT) && y == 0)
            return set_exception(ex, line, "division by zero");
        switch (op) {
        case TOK_PLUS: z = x + y; break;
        case TOK_MINUS: z = x - y; break;
        case TOK_STAR: z = x * y; break;
        case TOK_SLASH: z = x / y; break;
        case TOK_PERCENT: z = x % y; break;
        default: return set_exception(ex, line, "unknown arithmetic operator");
        }
        *out = as_value_int(z);
        return 0;
    }
    }
}

/* Apply a compound-assignment operator op (e.g. TOK_PLUS_ASSIGN) to a
 * current value and a right-hand value. */
static int eval_compound(as_exec *ex, int line, int op, const as_value *cur,
                         const as_value *rhs, as_value *out)
{
    int binop;
    switch (op) {
    case TOK_PLUS_ASSIGN: binop = TOK_PLUS; break;
    case TOK_MINUS_ASSIGN: binop = TOK_MINUS; break;
    case TOK_STAR_ASSIGN: binop = TOK_STAR; break;
    case TOK_SLASH_ASSIGN: binop = TOK_SLASH; break;
    case TOK_PERCENT_ASSIGN: binop = TOK_PERCENT; break;
    case TOK_SHL_ASSIGN: binop = TOK_SHL; break;
    case TOK_SHR_ASSIGN: binop = TOK_SHR; break;
    case TOK_BITAND_ASSIGN: binop = TOK_BITAND; break;
    case TOK_BITOR_ASSIGN: binop = TOK_BITOR; break;
    case TOK_BITXOR_ASSIGN: binop = TOK_BITXOR; break;
    default: return set_exception(ex, line, "unknown compound assignment");
    }
    return eval_binop(ex, line, binop, cur, rhs, out);
}

/* ---------------- expressions ---------------- */

static int eval_expr(as_exec *ex, as_scope *scope, as_ast *n, as_value *out);

static int eval_call(as_exec *ex, as_scope *scope, as_ast *n, as_value *out)
{
    as_value args[AS_MAX_ARGS];
    int nargs = n->nkids, i;
    if (nargs > AS_MAX_ARGS)
        return set_exception(ex, n->line, "too many arguments in call");
    for (i = 0; i < nargs; i++) {
        if (eval_expr(ex, scope, n->kids[i], &args[i]) != 0) return -1;
    }
    if (check_bounds(ex, n->line) != 0) return -1;
    /* script function first, then registered global function */
    {
        struct asIScriptFunction *fn =
            as_engine_find_script_func(ex->module, n->name, nargs);
        if (fn) {
            if (ex->call_depth >= ex->ctx->max_call_depth)
                return set_exception(ex, n->line, "call depth limit exceeded");
            return as_exec_function(ex, fn, args, nargs, out);
        }
    }
    {
        as_reg_func *rf = as_engine_find_reg_func(ex->engine, n->name, nargs);
        if (rf) {
            return as_engine_call_registered(ex->ctx, rf, args, nargs, out);
        }
    }
    {
        char buf[200];
        snprintf(buf, sizeof buf, "unknown function '%s'", n->name);
        return set_exception(ex, n->line, buf);
    }
}

static int eval_expr(as_exec *ex, as_scope *scope, as_ast *n, as_value *out)
{
    switch (n->kind) {
    case AST_LIT_INT: *out = as_value_int((int32_t)n->ival); return 0;
    case AST_LIT_INT64: *out = as_value_int64(n->ival); return 0;
    case AST_LIT_UINT: *out = as_value_uint((uint32_t)n->uval); return 0;
    case AST_LIT_UINT64: *out = as_value_uint64(n->uval); return 0;
    case AST_LIT_FLOAT: *out = as_value_float((float)n->fval); return 0;
    case AST_LIT_DOUBLE: *out = as_value_double(n->fval); return 0;
    case AST_LIT_BOOL: *out = as_value_bool(n->bval); return 0;
    case AST_IDENT: {
        as_var *v = scope_lookup(scope, n->name);
        if (!v) {
            char buf[200];
            snprintf(buf, sizeof buf, "undefined variable '%s'", n->name);
            return set_exception(ex, n->line, buf);
        }
        *out = v->val;
        return 0;
    }
    case AST_CALL:
        return eval_call(ex, scope, n, out);
    case AST_BINOP: {
        if (n->op == TOK_AND || n->op == TOK_OR) {
            as_value l, r;
            if (eval_expr(ex, scope, n->kids[0], &l) != 0) return -1;
            if (n->op == TOK_AND) {
                if (!as_value_truthy(&l)) { *out = as_value_bool(0); return 0; }
                if (eval_expr(ex, scope, n->kids[1], &r) != 0) return -1;
                *out = as_value_bool(as_value_truthy(&r));
                return 0;
            } else {
                if (as_value_truthy(&l)) { *out = as_value_bool(1); return 0; }
                if (eval_expr(ex, scope, n->kids[1], &r) != 0) return -1;
                *out = as_value_bool(as_value_truthy(&r));
                return 0;
            }
        }
        {
            as_value l, r;
            if (eval_expr(ex, scope, n->kids[0], &l) != 0) return -1;
            if (eval_expr(ex, scope, n->kids[1], &r) != 0) return -1;
            return eval_binop(ex, n->line, n->op, &l, &r, out);
        }
    }
    case AST_UNOP: {
        if (n->op == TOK_INC || n->op == TOK_DEC) {
            as_ast *t = n->kids[0];
            as_var *v;
            as_value nv, one, old;
            if (t->kind != AST_IDENT)
                return set_exception(ex, n->line, "'++'/'--' requires a variable");
            v = scope_lookup(scope, t->name);
            if (!v) {
                char buf[200];
                snprintf(buf, sizeof buf, "undefined variable '%s'", t->name);
                return set_exception(ex, n->line, buf);
            }
            if (!as_type_is_integer(v->type) && !as_type_is_float(v->type))
                return set_exception(ex, n->line, "'++'/'--' requires a numeric variable");
            old = v->val;
            one = as_value_int(1);
            if (eval_binop(ex, n->line, n->op == TOK_INC ? TOK_PLUS : TOK_MINUS,
                           &v->val, &one, &nv) != 0) return -1;
            if (as_value_cast(&nv, v->type, &v->val) != 0)
                return set_exception(ex, n->line, "invalid value for '++'/'--'");
            /* postfix returns the old value, prefix the new one */
            *out = n->aux ? old : v->val;
            return 0;
        }
        {
            as_value v;
            if (eval_expr(ex, scope, n->kids[0], &v) != 0) return -1;
            switch (n->op) {
            case TOK_MINUS: {
                as_value zero = as_value_int(0), r;
                if (eval_binop(ex, n->line, TOK_MINUS, &zero, &v, &r) != 0) return -1;
                *out = r;
                return 0;
            }
            case TOK_NOT:
                *out = as_value_bool(!as_value_truthy(&v));
                return 0;
            case TOK_BITNOT: {
                if (!as_type_is_integer(v.kind))
                    return set_exception(ex, n->line, "'~' requires an integer operand");
                switch (v.kind) {
                case AS_T_INT64: *out = as_value_int64(~v.v.i64); return 0;
                case AS_T_UINT64: *out = as_value_uint64(~v.v.u64); return 0;
                case AS_T_UINT: *out = as_value_uint(~v.v.u32); return 0;
                default: *out = as_value_int(~v.v.i32); return 0;
                }
            }
            default:
                return set_exception(ex, n->line, "unknown unary operator");
            }
        }
    }
    case AST_CAST: {
        as_value v;
        if (eval_expr(ex, scope, n->kids[0], &v) != 0) return -1;
        if (as_value_cast(&v, (as_type)n->type, out) != 0)
            return set_exception(ex, n->line, "invalid cast");
        return 0;
    }
    case AST_ASSIGN: {
        as_ast *t = n->kids[0];
        as_var *v;
        as_value rhs, res;
        if (t->kind != AST_IDENT)
            return set_exception(ex, n->line, "assignment target must be a variable");
        v = scope_lookup(scope, t->name);
        if (!v) {
            char buf[200];
            snprintf(buf, sizeof buf, "undefined variable '%s'", t->name);
            return set_exception(ex, n->line, buf);
        }
        if (eval_expr(ex, scope, n->kids[1], &rhs) != 0) return -1;
        if (n->op == TOK_ASSIGN) {
            res = rhs;
        } else {
            if (eval_compound(ex, n->line, n->op, &v->val, &rhs, &res) != 0) return -1;
        }
        if (as_value_cast(&res, v->type, &v->val) != 0)
            return set_exception(ex, n->line, "cannot assign value to variable");
        *out = v->val;
        return 0;
    }
    default:
        return set_exception(ex, n->line, "internal error: bad expression node");
    }
}

/* ---------------- statements ---------------- */

static int exec_stmt(as_exec *ex, as_scope *scope, as_ast *n);

/* Execute a block's statements in a NEW child scope. Returns a flow
 * signal, or -1 on exception. */
static int exec_block(as_exec *ex, as_scope *parent, as_ast *n)
{
    as_scope *s = scope_new(parent);
    int i, flow = FLOW_NORMAL;
    for (i = 0; i < n->nkids && flow == FLOW_NORMAL; i++) {
        int f = exec_stmt(ex, s, n->kids[i]);
        if (f < 0) { scope_free(s); return -1; }
        flow = f;
    }
    scope_free(s);
    return flow;
}

static int exec_stmt(as_exec *ex, as_scope *scope, as_ast *n)
{
    int i;
    if (check_bounds(ex, n->line) != 0) return -1;
    switch (n->kind) {
    case AST_BLOCK:
        return exec_block(ex, scope, n);
    case AST_VARDECL: {
        as_var *v;
        as_value init;
        v = scope_define(scope, n->name, (as_type)n->type);
        if (!v) {
            char buf[200];
            snprintf(buf, sizeof buf, "variable '%s' is already declared in this scope", n->name);
            return set_exception(ex, n->line, buf);
        }
        if (n->nkids > 0) {
            if (eval_expr(ex, scope, n->kids[0], &init) != 0) return -1;
            if (as_value_cast(&init, v->type, &v->val) != 0)
                return set_exception(ex, n->line, "cannot initialize variable with this value");
        }
        return FLOW_NORMAL;
    }
    case AST_IF: {
        as_value c;
        if (eval_expr(ex, scope, n->kids[0], &c) != 0) return -1;
        if (as_value_truthy(&c)) return exec_stmt(ex, scope, n->kids[1]);
        if (n->nkids > 2) return exec_stmt(ex, scope, n->kids[2]);
        return FLOW_NORMAL;
    }
    case AST_WHILE: {
        for (;;) {
            as_value c;
            int f;
            if (check_bounds(ex, n->line) != 0) return -1;
            if (eval_expr(ex, scope, n->kids[0], &c) != 0) return -1;
            if (!as_value_truthy(&c)) return FLOW_NORMAL;
            f = exec_stmt(ex, scope, n->kids[1]);
            if (f < 0) return -1;
            if (f == FLOW_BREAK) return FLOW_NORMAL;
            if (f == FLOW_CONTINUE) continue;
            if (f == FLOW_RETURN) return FLOW_RETURN;
        }
    }
    case AST_FOR: {
        as_scope *fs = scope_new(scope);
        int flow = FLOW_NORMAL;
        /* init: a VARDECL list (BLOCK aux==1) or a single statement/expr */
        if (n->kids[0]) {
            if (n->kids[0]->kind == AST_BLOCK && n->kids[0]->aux == 1) {
                for (i = 0; i < n->kids[0]->nkids; i++) {
                    int f = exec_stmt(ex, fs, n->kids[0]->kids[i]);
                    if (f < 0) { scope_free(fs); return -1; }
                    if (f == FLOW_RETURN) { scope_free(fs); return FLOW_RETURN; }
                }
            } else if (n->kids[0]->kind == AST_VARDECL) {
                int f = exec_stmt(ex, fs, n->kids[0]);
                if (f < 0) { scope_free(fs); return -1; }
                if (f == FLOW_RETURN) { scope_free(fs); return FLOW_RETURN; }
            } else {
                as_value dummy;
                if (eval_expr(ex, fs, n->kids[0], &dummy) != 0) { scope_free(fs); return -1; }
            }
        }
        for (;;) {
            as_value c;
            int f;
            if (check_bounds(ex, n->line) != 0) { scope_free(fs); return -1; }
            if (n->kids[1]) {
                if (eval_expr(ex, fs, n->kids[1], &c) != 0) { scope_free(fs); return -1; }
                if (!as_value_truthy(&c)) break;
            }
            f = exec_stmt(ex, fs, n->kids[3]);
            if (f < 0) { scope_free(fs); return -1; }
            if (f == FLOW_BREAK) break;
            if (f == FLOW_RETURN) { scope_free(fs); return FLOW_RETURN; }
            /* FLOW_CONTINUE falls through to the increment */
            if (n->kids[2]) {
                as_value dummy;
                if (eval_expr(ex, fs, n->kids[2], &dummy) != 0) { scope_free(fs); return -1; }
            }
            flow = FLOW_NORMAL;
        }
        scope_free(fs);
        return flow;
    }
    case AST_RETURN: {
        if (n->nkids > 0) {
            as_value v;
            if (eval_expr(ex, scope, n->kids[0], &v) != 0) return -1;
            ex->ctx->retval = v;
            ex->ctx->has_retval = 1;
        }
        return FLOW_RETURN;
    }
    case AST_BREAK: return FLOW_BREAK;
    case AST_CONTINUE: return FLOW_CONTINUE;
    case AST_EXPRSTMT: {
        as_value dummy;
        if (n->nkids > 0) {
            if (eval_expr(ex, scope, n->kids[0], &dummy) != 0) return -1;
        }
        return FLOW_NORMAL;
    }
    default:
        return set_exception(ex, n->line, "internal error: bad statement node");
    }
}

/* ---------------- function execution ---------------- */

int as_exec_function(as_exec *ex, struct asIScriptFunction *fn,
                     as_value *args, int nargs, as_value *ret)
{
    as_ast *fnode = fn->ast;
    as_ast *params, *body;
    as_scope *s;
    int i, flow;
    as_value r;
    if (!fnode || fnode->kind != AST_FUNC)
        return set_exception(ex, 0, "internal error: function has no body");
    params = fnode->kids[0];
    body = fnode->kids[1];
    if (nargs != fn->nparams) {
        char buf[200];
        snprintf(buf, sizeof buf, "function '%s' expects %d argument(s), got %d",
                 fn->name, fn->nparams, nargs);
        return set_exception(ex, fnode->line, buf);
    }
    s = scope_new(NULL);
    for (i = 0; i < nargs; i++) {
        as_ast *pnode = params->kids[i];
        as_var *v = scope_define(s, pnode->name, (as_type)pnode->type);
        as_value cv;
        if (!v) { scope_free(s); return set_exception(ex, fnode->line, "duplicate parameter name"); }
        if (as_value_cast(&args[i], (as_type)pnode->type, &cv) != 0) {
            scope_free(s);
            return set_exception(ex, fnode->line, "cannot convert argument to parameter type");
        }
        v->val = cv;
    }
    ex->call_depth++;
    ex->ctx->has_retval = 0;
    flow = exec_block(ex, s, body);
    ex->call_depth--;
    scope_free(s);
    if (flow < 0) return -1;
    if (fn->ret == AS_T_VOID) {
        *ret = as_value_void();
        return 0;
    }
    if (!ex->ctx->has_retval) {
        /* fell off the end without return: default zero value */
        r = as_value_int(0);
        if (as_value_cast(&r, fn->ret, ret) != 0)
            return set_exception(ex, fnode->line, "cannot produce a default return value");
        return 0;
    }
    if (as_value_cast(&ex->ctx->retval, fn->ret, ret) != 0)
        return set_exception(ex, fnode->line, "return value does not match the declared return type");
    return 0;
}

/* ---------------- validator ---------------- */
/* Compile-time name resolution: variables must be declared in an
 * enclosing scope; calls must target a script function of the module
 * or a registered global function. */

typedef struct {
    struct asIScriptEngine *engine;
    struct asIScriptModule *module;
    char errmsg[256];
    int errline;
    int failed;
} as_validator;

typedef struct as_vscope {
    struct as_vscope *parent;
    char **names;
    int nnames, capnames;
} as_vscope;

static as_vscope *vscope_new(as_vscope *parent)
{
    as_vscope *s = as_xmalloc(sizeof *s);
    memset(s, 0, sizeof *s);
    s->parent = parent;
    return s;
}

static void vscope_free(as_vscope *s)
{
    int i;
    for (i = 0; i < s->nnames; i++) free(s->names[i]);
    free(s->names);
    free(s);
}

static int vscope_define(as_vscope *s, const char *name)
{
    int i;
    for (i = 0; i < s->nnames; i++)
        if (strcmp(s->names[i], name) == 0) return -1;
    if (s->nnames >= s->capnames) {
        s->capnames = s->capnames ? s->capnames * 2 : 8;
        s->names = as_xrealloc(s->names, (size_t)s->capnames * sizeof *s->names);
    }
    s->names[s->nnames++] = as_xstrdup(name);
    return 0;
}

static int vscope_lookup(as_vscope *s, const char *name)
{
    int i;
    for (; s; s = s->parent) {
        for (i = 0; i < s->nnames; i++)
            if (strcmp(s->names[i], name) == 0) return 1;
    }
    return 0;
}

static void vfail(as_validator *v, int line, const char *msg)
{
    if (v->failed) return;
    v->failed = 1;
    v->errline = line;
    snprintf(v->errmsg, sizeof v->errmsg, "%s", msg);
}

static int validate_expr(as_validator *v, as_vscope *s, as_ast *n);
static int validate_stmt(as_validator *v, as_vscope *s, as_ast *n);

static int validate_expr(as_validator *v, as_vscope *s, as_ast *n)
{
    int i;
    if (v->failed) return -1;
    switch (n->kind) {
    case AST_LIT_INT: case AST_LIT_INT64: case AST_LIT_UINT:
    case AST_LIT_UINT64: case AST_LIT_FLOAT: case AST_LIT_DOUBLE:
    case AST_LIT_BOOL:
        return 0;
    case AST_IDENT:
        if (!vscope_lookup(s, n->name)) {
            char buf[200];
            snprintf(buf, sizeof buf, "undefined variable '%s'", n->name);
            vfail(v, n->line, buf);
            return -1;
        }
        return 0;
    case AST_CALL: {
        int found = 0;
        for (i = 0; i < v->module->nfuncs; i++) {
            if (strcmp(v->module->funcs[i]->name, n->name) == 0 &&
                v->module->funcs[i]->nparams == n->nkids) { found = 1; break; }
        }
        if (!found) {
            for (i = 0; i < v->engine->nregfuncs; i++) {
                if (strcmp(v->engine->regfuncs[i].name, n->name) == 0 &&
                    v->engine->regfuncs[i].nparams == n->nkids) { found = 1; break; }
            }
        }
        if (!found) {
            char buf[200];
            snprintf(buf, sizeof buf, "unknown function '%s'", n->name);
            vfail(v, n->line, buf);
            return -1;
        }
        for (i = 0; i < n->nkids; i++)
            if (validate_expr(v, s, n->kids[i]) != 0) return -1;
        return 0;
    }
    case AST_BINOP: case AST_ASSIGN:
        for (i = 0; i < n->nkids; i++)
            if (validate_expr(v, s, n->kids[i]) != 0) return -1;
        return 0;
    case AST_UNOP:
    case AST_CAST:
        return validate_expr(v, s, n->kids[0]);
    default:
        vfail(v, n->line, "internal error: bad expression node in validator");
        return -1;
    }
}

static int validate_stmt(as_validator *v, as_vscope *s, as_ast *n)
{
    int i;
    if (v->failed) return -1;
    switch (n->kind) {
    case AST_BLOCK: {
        as_vscope *inner = vscope_new(s);
        for (i = 0; i < n->nkids; i++) {
            if (validate_stmt(v, inner, n->kids[i]) != 0) { vscope_free(inner); return -1; }
        }
        vscope_free(inner);
        return 0;
    }
    case AST_VARDECL:
        if (n->nkids > 0 && validate_expr(v, s, n->kids[0]) != 0) return -1;
        if (vscope_define(s, n->name) != 0) {
            char buf[200];
            snprintf(buf, sizeof buf, "variable '%s' is already declared in this scope", n->name);
            vfail(v, n->line, buf);
            return -1;
        }
        return 0;
    case AST_IF:
        if (validate_expr(v, s, n->kids[0]) != 0) return -1;
        if (validate_stmt(v, s, n->kids[1]) != 0) return -1;
        if (n->nkids > 2 && validate_stmt(v, s, n->kids[2]) != 0) return -1;
        return 0;
    case AST_WHILE:
        if (validate_expr(v, s, n->kids[0]) != 0) return -1;
        return validate_stmt(v, s, n->kids[1]);
    case AST_FOR: {
        as_vscope *fs = vscope_new(s);
        if (n->kids[0]) {
            if (n->kids[0]->kind == AST_BLOCK && n->kids[0]->aux == 1) {
                for (i = 0; i < n->kids[0]->nkids; i++)
                    if (validate_stmt(v, fs, n->kids[0]->kids[i]) != 0) { vscope_free(fs); return -1; }
            } else if (n->kids[0]->kind == AST_VARDECL) {
                if (validate_stmt(v, fs, n->kids[0]) != 0) { vscope_free(fs); return -1; }
            } else {
                if (validate_expr(v, fs, n->kids[0]) != 0) { vscope_free(fs); return -1; }
            }
        }
        if (n->kids[1] && validate_expr(v, fs, n->kids[1]) != 0) { vscope_free(fs); return -1; }
        if (n->kids[2] && validate_expr(v, fs, n->kids[2]) != 0) { vscope_free(fs); return -1; }
        if (validate_stmt(v, fs, n->kids[3]) != 0) { vscope_free(fs); return -1; }
        vscope_free(fs);
        return 0;
    }
    case AST_RETURN:
        if (n->nkids > 0) return validate_expr(v, s, n->kids[0]);
        return 0;
    case AST_BREAK: case AST_CONTINUE:
        return 0;
    case AST_EXPRSTMT:
        if (n->nkids > 0) return validate_expr(v, s, n->kids[0]);
        return 0;
    default:
        vfail(v, n->line, "internal error: bad statement node in validator");
        return -1;
    }
}

int as_validate_program(struct asIScriptEngine *eng,
                        struct asIScriptModule *mod,
                        as_ast *prog, char *errmsg, size_t errcap,
                        int *errline)
{
    as_validator v;
    int i;
    memset(&v, 0, sizeof v);
    v.engine = eng;
    v.module = mod;
    for (i = 0; i < prog->nkids && !v.failed; i++) {
        as_ast *fn = prog->kids[i];
        as_ast *params = fn->kids[0];
        as_ast *body = fn->kids[1];
        as_vscope *s = vscope_new(NULL);
        int j;
        for (j = 0; j < params->nkids; j++) {
            if (vscope_define(s, params->kids[j]->name) != 0) {
                vfail(&v, fn->line, "duplicate parameter name");
                break;
            }
        }
        if (!v.failed) validate_stmt(&v, s, body);
        vscope_free(s);
    }
    if (v.failed) {
        snprintf(errmsg, errcap, "%s", v.errmsg);
        *errline = v.errline;
        return -1;
    }
    return 0;
}
