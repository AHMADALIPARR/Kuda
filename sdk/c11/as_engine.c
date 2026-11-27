// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* as_engine.c - C API implementation (angelscript_c.h) on the
 * reverse-flow-machine execution spine (ffi/bridge.h).
 *
 * Mapping onto the bridge:
 *   - Each context embeds an rf_machine_t. asContext_Prepare initialises
 *     it (fuel = step budget + lifecycle headroom), registers the
 *     lifecycle edges, and steps PENDING -> VALIDATED.
 *   - asContext_Execute runs the tree-walking interpreter; every
 *     interpreter step burns one unit of rf.fuel (the step budget).
 *     On normal exit it steps VALIDATED -> COMMITTED and returns
 *     asEXECUTION_FINISHED; on exception/abort it steps VALIDATED ->
 *     REJECTED and returns asEXECUTION_EXCEPTION / asEXECUTION_ABORTED.
 *   - The rf trace records the lifecycle transitions; rf_replay_verify
 *     validates them (phase admissibility + strictly decreasing fuel).
 *   - The machine struct + trace are the serialisable checkpoint.
 *
 * The host function registry (name -> C function pointer + signature)
 * and the thin ABI shim live here. The AngelScript front end (lexer,
 * parser, AST, interpreter) is as_lexer.c / as_parser.c / as_interp.c.
 * MUMPS/INTERCAL reverse control flow stays internal: script syntax is
 * normal C-like procedural code only (no COME FROM in scripts).
 *
 * Pure C11. No C++, no SDK, no third-party code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "as_c11.h"
#include "as_tramp.h"

/* Bridge lifecycle edge labels (internal; never exposed to scripts). */
#define RF_LABEL_PREPARE   100
#define RF_LABEL_VALIDATED 101
#define RF_LABEL_EXEC_OK   102
#define RF_LABEL_COMMITTED 103
#define RF_LABEL_EXEC_FAIL 104
#define RF_LABEL_REJECTED  105

/* ------------------------------------------------------------------ */
/* Diagnostics                                                         */
/* ------------------------------------------------------------------ */

void as_engine_message(struct asIScriptEngine *e, const char *section,
                       int row, int col, int type, const char *msg)
{
    if (e && e->msg_cb) {
        asSMessageInfo info;
        info.section = section ? section : "";
        info.row = row;
        info.col = col;
        info.type = type;
        info.message = msg ? msg : "";
        e->msg_cb(&info, e->msg_cb_obj);
    }
}

static void engine_error(struct asIScriptEngine *e, const char *section,
                         int row, const char *msg)
{
    as_engine_message(e, section, row, 0, asMSGTYPE_ERROR, msg);
}

/* ------------------------------------------------------------------ */
/* Allocation helpers                                                  */
/* ------------------------------------------------------------------ */

static void free_script_function(struct asIScriptFunction *fn)
{
    if (!fn) return;
    free(fn->name);
    free(fn->decl);
    free(fn->params);
    /* fn->ast is owned by the module's program AST; not freed here. */
    free(fn);
}

static void module_clear(struct asIScriptModule *m)
{
    int i;
    for (i = 0; i < m->nfuncs; i++) free_script_function(m->funcs[i]);
    free(m->funcs);
    m->funcs = NULL;
    m->nfuncs = 0;
    m->capfuncs = 0;
    as_ast_free(m->program);
    m->program = NULL;
    m->built = 0;
}

static void free_module(struct asIScriptModule *m)
{
    int i;
    if (!m) return;
    module_clear(m);
    for (i = 0; i < m->nsections; i++) {
        free(m->sections[i].name);
        free(m->sections[i].code);
    }
    free(m->sections);
    free(m->name);
    free(m);
}

static void engine_remove_module(struct asIScriptEngine *e,
                                 struct asIScriptModule *m)
{
    int i;
    for (i = 0; i < e->nmodules; i++) {
        if (e->modules[i] == m) {
            memmove(&e->modules[i], &e->modules[i + 1],
                    (size_t)(e->nmodules - i - 1) * sizeof *e->modules);
            e->nmodules--;
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Engine                                                              */
/* ------------------------------------------------------------------ */

asIScriptEngine *asCreateScriptEngine(asDWORD version)
{
    asIScriptEngine *e;
    if (version != (asDWORD)ANGELSCRIPT_VERSION) return NULL;
    e = as_xmalloc(sizeof *e);
    memset(e, 0, sizeof *e);
    e->refCount = 1;
    return e;
}

int asEngine_AddRef(asIScriptEngine *e)
{
    if (!e) return asINVALID_ARG;
    return ++e->refCount;
}

int asEngine_Release(asIScriptEngine *e)
{
    int i;
    if (!e) return asINVALID_ARG;
    if (--e->refCount > 0) return e->refCount;
    /* Engine is going away; force-free any modules still registered.
     * Callers must release modules/contexts before the engine. */
    for (i = 0; i < e->nmodules; i++) free_module(e->modules[i]);
    free(e->modules);
    for (i = 0; i < e->nregfuncs; i++) {
        free(e->regfuncs[i].decl);
        free(e->regfuncs[i].name);
        free(e->regfuncs[i].params);
    }
    free(e->regfuncs);
    free(e);
    return 0;
}

int asEngine_SetMessageCallback(asIScriptEngine *e, asMESSAGEFUNC_t callback,
                                void *obj, asDWORD callConv)
{
    if (!e) return asINVALID_ARG;
    (void)callConv;
    e->msg_cb = callback;
    e->msg_cb_obj = obj;
    e->msg_callconv = callConv;
    return asSUCCESS;
}

int asEngine_ClearMessageCallback(asIScriptEngine *e)
{
    if (!e) return asINVALID_ARG;
    e->msg_cb = NULL;
    e->msg_cb_obj = NULL;
    return asSUCCESS;
}

int asEngine_RegisterGlobalFunction(asIScriptEngine *e, const char *declaration,
                                    asFUNCTION_t funcPointer, asDWORD callConv)
{
    as_decl d;
    as_reg_func *rf;
    int i;
    if (!e || !declaration || !funcPointer) return asINVALID_ARG;
    if (callConv != asCALL_CDECL && callConv != asCALL_GENERIC &&
        callConv != asCALL_STDCALL)
        return asNOT_SUPPORTED;
    if (as_parse_decl(declaration, &d) != 0) {
        engine_error(e, "", 0, d.errmsg);
        as_decl_free(&d);
        return asINVALID_DECLARATION;
    }
    /* duplicate registration (same name + same signature) is an error */
    for (i = 0; i < e->nregfuncs; i++) {
        as_reg_func *o = &e->regfuncs[i];
        if (strcmp(o->name, d.name) == 0 && o->nparams == d.nparams &&
            o->ret == d.ret) {
            int same = 1, j;
            for (j = 0; j < d.nparams; j++)
                if (o->params[j] != d.params[j]) { same = 0; break; }
            if (same) {
                as_decl_free(&d);
                return asALREADY_REGISTERED;
            }
        }
    }
    if (e->nregfuncs >= e->capregfuncs) {
        e->capregfuncs = e->capregfuncs ? e->capregfuncs * 2 : 8;
        e->regfuncs = as_xrealloc(e->regfuncs,
                                  (size_t)e->capregfuncs * sizeof *e->regfuncs);
    }
    rf = &e->regfuncs[e->nregfuncs];
    memset(rf, 0, sizeof *rf);
    rf->decl = as_xstrdup(declaration);
    rf->name = d.name; /* steal */
    rf->ret = d.ret;
    rf->params = d.params; /* steal */
    rf->nparams = d.nparams;
    rf->func = funcPointer;
    rf->callConv = callConv;
    rf->id = e->nregfuncs;
    d.name = NULL;
    d.params = NULL;
    as_decl_free(&d);
    e->nregfuncs++;
    return rf->id;
}

int asEngine_RegisterObjectType(asIScriptEngine *e, const char *name,
                                int byteSize, asDWORD flags)
{
    (void)e; (void)name; (void)byteSize; (void)flags;
    return asNOT_SUPPORTED; /* no classes/objects in this subset */
}

int asEngine_RegisterObjectMethod(asIScriptEngine *e, const char *obj,
                                  const char *declaration,
                                  asFUNCTION_t funcPointer, asDWORD callConv)
{
    (void)e; (void)obj; (void)declaration; (void)funcPointer; (void)callConv;
    return asNOT_SUPPORTED;
}

int asEngine_RegisterObjectProperty(asIScriptEngine *e, const char *obj,
                                    const char *declaration, int byteOffset)
{
    (void)e; (void)obj; (void)declaration; (void)byteOffset;
    return asNOT_SUPPORTED;
}

int asEngine_RegisterObjectBehaviour(asIScriptEngine *e, const char *obj,
                                     asDWORD behaviour, const char *declaration,
                                     asFUNCTION_t funcPointer, asDWORD callConv)
{
    (void)e; (void)obj; (void)behaviour; (void)declaration;
    (void)funcPointer; (void)callConv;
    return asNOT_SUPPORTED;
}

asIScriptModule *asEngine_GetModule(asIScriptEngine *e, const char *module,
                                    asDWORD flag)
{
    int i;
    struct asIScriptModule *m = NULL;
    if (!e || !module) return NULL;
    for (i = 0; i < e->nmodules; i++) {
        if (strcmp(e->modules[i]->name, module) == 0) { m = e->modules[i]; break; }
    }
    if (flag == asGM_ONLY_IF_EXISTS) {
        if (m) m->refCount++;
        return m;
    }
    if (flag == asGM_CREATE_IF_NOT_EXISTS) {
        if (m) { m->refCount++; return m; }
    }
    if (flag == asGM_ALWAYS_CREATE) {
        if (m) {
            /* discard the old module with this name */
            engine_remove_module(e, m);
            /* drop our registry reference; the caller's reference (if any)
             * is their responsibility */
            if (--m->refCount <= 0) free_module(m);
            m = NULL;
        }
    }
    if (flag != asGM_ONLY_IF_EXISTS && flag != asGM_CREATE_IF_NOT_EXISTS &&
        flag != asGM_ALWAYS_CREATE)
        return NULL;
    m = as_xmalloc(sizeof *m);
    memset(m, 0, sizeof *m);
    m->refCount = 1;
    m->engine = e;
    m->name = as_xstrdup(module);
    if (e->nmodules >= e->capmodules) {
        e->capmodules = e->capmodules ? e->capmodules * 2 : 4;
        e->modules = as_xrealloc(e->modules,
                                 (size_t)e->capmodules * sizeof *e->modules);
    }
    e->modules[e->nmodules++] = m;
    return m;
}

int asEngine_DiscardModule(asIScriptEngine *e, const char *module)
{
    int i;
    if (!e || !module) return asINVALID_ARG;
    for (i = 0; i < e->nmodules; i++) {
        if (strcmp(e->modules[i]->name, module) == 0) {
            struct asIScriptModule *m = e->modules[i];
            engine_remove_module(e, m);
            if (--m->refCount <= 0) free_module(m);
            return asSUCCESS;
        }
    }
    return asERROR;
}

asIScriptContext *asEngine_CreateContext(asIScriptEngine *e)
{
    asIScriptContext *c;
    if (!e) return NULL;
    c = as_xmalloc(sizeof *c);
    memset(c, 0, sizeof *c);
    c->refCount = 1;
    c->engine = e;
    c->state = asEXECUTION_UNINITIALIZED;
    c->max_steps = AS_DEFAULT_MAX_STEPS;
    c->max_call_depth = AS_DEFAULT_MAX_CALL_DEPTH;
    c->retval = as_value_void();
    rf_init(&c->rf, (int32_t)(c->max_steps + 2));
    return c;
}

const char *asEngine_GetLibraryVersion(void)
{
    return ANGELSCRIPT_VERSION_STRING;
}

const char *asEngine_GetLibraryOptions(void)
{
    return "C11 subset: tree-walking interpreter, reverse-flow-machine "
           "execution spine, no JIT, no GC, no classes";
}

/* ------------------------------------------------------------------ */
/* Module                                                              */
/* ------------------------------------------------------------------ */

int asModule_AddRef(asIScriptModule *m)
{
    if (!m) return asINVALID_ARG;
    return ++m->refCount;
}

int asModule_Release(asIScriptModule *m)
{
    struct asIScriptEngine *e;
    if (!m) return asINVALID_ARG;
    e = m->engine;
    if (--m->refCount > 0) return m->refCount;
    if (e) engine_remove_module(e, m);
    free_module(m);
    return 0;
}

int asModule_AddScriptSection(asIScriptModule *m, const char *name,
                              const char *code, size_t codeLength,
                              int lineOffset)
{
    struct as_section *s;
    size_t len;
    if (!m || !code) return asINVALID_ARG;
    len = codeLength ? codeLength : strlen(code);
    if (m->nsections >= m->capsections) {
        m->capsections = m->capsections ? m->capsections * 2 : 4;
        m->sections = as_xrealloc(m->sections,
                                  (size_t)m->capsections * sizeof *m->sections);
    }
    s = &m->sections[m->nsections];
    memset(s, 0, sizeof *s);
    s->name = as_xstrdup(name ? name : "");
    s->code = as_xmalloc(len + 1);
    memcpy(s->code, code, len);
    s->code[len] = '\0';
    s->len = len;
    s->lineOffset = lineOffset;
    m->nsections++;
    m->built = 0; /* a new section invalidates the build */
    return asSUCCESS;
}

/* Build a canonical declaration string for a script function.
 * Fixed 512-byte buffer: a declaration (name + up to AS_MAX_ARGS short
 * type names) is far shorter than that. */
static char *build_canonical_decl(as_type ret, const char *name,
                                  as_ast *params)
{
    char *buf = as_xmalloc(512);
    size_t n = 0;
    int i;
    n += (size_t)snprintf(buf + n, 512 - n, "%s %s(",
                          as_type_name(ret), name);
    for (i = 0; i < params->nkids; i++) {
        as_ast *pnode = params->kids[i];
        if (i > 0) n += (size_t)snprintf(buf + n, 512 - n, ", ");
        n += (size_t)snprintf(buf + n, 512 - n, "%s",
                              as_type_name((as_type)pnode->type));
    }
    snprintf(buf + n, 512 - n, ")");
    return buf;
}

int asModule_Build(asIScriptModule *m)
{
    as_parser p;
    as_ast *prog = NULL;
    char *src = NULL;
    size_t total = 0, off = 0;
    int i, rc = asERROR;
    char errmsg[256];
    int errline = 0;
    if (!m) return asINVALID_ARG;
    if (m->nsections == 0) {
        engine_error(m->engine, m->name, 0, "module has no script sections");
        return asERROR;
    }
    /* Concatenate sections in order (newline-separated). */
    for (i = 0; i < m->nsections; i++) total += m->sections[i].len + 1;
    src = as_xmalloc(total + 1);
    for (i = 0; i < m->nsections; i++) {
        memcpy(src + off, m->sections[i].code, m->sections[i].len);
        off += m->sections[i].len;
        src[off++] = '\n';
    }
    src[off] = '\0';
    module_clear(m);
    as_parser_init(&p);
    prog = as_parse_program(&p, src, m->nsections > 0 ? m->sections[0].name : "");
    if (!prog) {
        snprintf(errmsg, sizeof errmsg, "%s", p.errmsg);
        errline = p.errline;
        engine_error(m->engine, m->nsections > 0 ? m->sections[0].name : "",
                     errline, errmsg);
        as_parser_free(&p);
        free(src);
        return asERROR;
    }
    as_parser_free(&p);
    m->program = prog;
    /* Build the function table from the parsed program. */
    for (i = 0; i < prog->nkids; i++) {
        as_ast *fnode = prog->kids[i];
        as_ast *params = fnode->kids[0];
        struct asIScriptFunction *fn;
        int j, dup = 0;
        for (j = 0; j < m->nfuncs; j++) {
            if (strcmp(m->funcs[j]->name, fnode->name) == 0 &&
                m->funcs[j]->nparams == params->nkids) { dup = 1; break; }
        }
        if (dup) {
            snprintf(errmsg, sizeof errmsg, "duplicate function '%s'", fnode->name);
            engine_error(m->engine, m->name, fnode->line, errmsg);
            free(src);
            return asERROR;
        }
        fn = as_xmalloc(sizeof *fn);
        memset(fn, 0, sizeof *fn);
        fn->refCount = 1;
        fn->module = m;
        fn->name = as_xstrdup(fnode->name);
        fn->ret = (as_type)fnode->type;
        fn->nparams = params->nkids;
        fn->params = as_xmalloc((size_t)(fn->nparams ? fn->nparams : 1) * sizeof *fn->params);
        for (j = 0; j < fn->nparams; j++)
            fn->params[j] = (as_type)params->kids[j]->type;
        fn->decl = build_canonical_decl(fn->ret, fn->name, params);
        fn->ast = fnode;
        fn->is_script = 1;
        if (m->nfuncs >= m->capfuncs) {
            m->capfuncs = m->capfuncs ? m->capfuncs * 2 : 8;
            m->funcs = as_xrealloc(m->funcs,
                                   (size_t)m->capfuncs * sizeof *m->funcs);
        }
        m->funcs[m->nfuncs++] = fn;
    }
    /* Compile-time validation: variables declared, calls resolvable. */
    if (as_validate_program(m->engine, m, prog, errmsg, sizeof errmsg, &errline) != 0) {
        engine_error(m->engine, m->nsections > 0 ? m->sections[0].name : "",
                     errline, errmsg);
        free(src);
        return asERROR;
    }
    m->built = 1;
    rc = asSUCCESS;
    free(src);
    return rc;
}

static int decl_matches(as_decl *d, struct asIScriptFunction *fn)
{
    int i;
    if (strcmp(d->name, fn->name) != 0) return 0;
    if (d->ret != fn->ret) return 0;
    if (d->nparams != fn->nparams) return 0;
    for (i = 0; i < d->nparams; i++)
        if (d->params[i] != fn->params[i]) return 0;
    return 1;
}

asIScriptFunction *asModule_GetFunctionByDecl(asIScriptModule *m, const char *decl)
{
    as_decl d;
    int i;
    if (!m || !decl || !m->built) return NULL;
    if (as_parse_decl(decl, &d) != 0) { as_decl_free(&d); return NULL; }
    for (i = 0; i < m->nfuncs; i++) {
        if (decl_matches(&d, m->funcs[i])) {
            as_decl_free(&d);
            return m->funcs[i];
        }
    }
    as_decl_free(&d);
    return NULL;
}

asIScriptFunction *asModule_GetFunctionByName(asIScriptModule *m, const char *name)
{
    int i;
    if (!m || !name || !m->built) return NULL;
    for (i = 0; i < m->nfuncs; i++) {
        if (strcmp(m->funcs[i]->name, name) == 0) return m->funcs[i];
    }
    return NULL;
}

int asModule_GetFunctionCount(asIScriptModule *m)
{
    if (!m || !m->built) return 0;
    return m->nfuncs;
}

/* ------------------------------------------------------------------ */
/* Function lookup helpers (used by the interpreter)                   */
/* ------------------------------------------------------------------ */

struct asIScriptFunction *as_engine_find_script_func(
    struct asIScriptModule *m, const char *name, int nargs)
{
    int i;
    if (!m || !name) return NULL;
    for (i = 0; i < m->nfuncs; i++) {
        if (strcmp(m->funcs[i]->name, name) == 0 &&
            m->funcs[i]->nparams == nargs)
            return m->funcs[i];
    }
    return NULL;
}

as_reg_func *as_engine_find_reg_func(struct asIScriptEngine *e,
                                     const char *name, int nargs)
{
    int i;
    if (!e || !name) return NULL;
    for (i = 0; i < e->nregfuncs; i++) {
        if (strcmp(e->regfuncs[i].name, name) == 0 &&
            e->regfuncs[i].nparams == nargs)
            return &e->regfuncs[i];
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Registered C function calls (trampoline)                            */
/* ------------------------------------------------------------------ */

static void ctx_exception(asIScriptContext *ctx, int line, const char *msg)
{
    snprintf(ctx->exception, sizeof ctx->exception, "%s", msg);
    ctx->exception_line = line;
    ctx->exception_col = 0;
    snprintf(ctx->exception_section, sizeof ctx->exception_section, "%s",
             ctx->func && ctx->func->module ? ctx->func->module->name : "");
}

int as_engine_call_registered(struct asIScriptContext *ctx,
                              as_reg_func *rf, as_value *args, int nargs,
                              as_value *ret)
{
    uint64_t gp[6];
    double fp[4];
    int ngp = 0, nfp = 0, i;
    if (!ctx || !rf) return -1;
    if (nargs != rf->nparams) {
        ctx_exception(ctx, 0, "wrong number of arguments for registered function");
        return -1;
    }
#if !defined(__x86_64__)
    /* Non-x86-64: only 0-argument void functions are supported. */
    if (nargs != 0 || rf->ret != AS_T_VOID) {
        ctx_exception(ctx, 0, "registered calls with arguments are not supported on this architecture");
        return -1;
    }
#endif
    memset(gp, 0, sizeof gp);
    memset(fp, 0, sizeof fp);
    for (i = 0; i < nargs; i++) {
        as_type pt = rf->params[i];
        as_value cv;
        if (pt == AS_T_VOID) {
            /* array/object parameter: cannot be marshalled from script */
            ctx_exception(ctx, 0, "cannot pass an array/object argument to a registered function");
            return -1;
        }
        if (as_value_cast(&args[i], pt, &cv) != 0) {
            ctx_exception(ctx, 0, "cannot convert argument to the declared parameter type");
            return -1;
        }
        switch (pt) {
        case AS_T_BOOL:
            if (ngp >= 6) goto too_many;
            gp[ngp++] = (uint64_t)(cv.v.b ? 1 : 0);
            break;
        case AS_T_INT:
            if (ngp >= 6) goto too_many;
            gp[ngp++] = (uint64_t)(int64_t)cv.v.i32;
            break;
        case AS_T_UINT:
            if (ngp >= 6) goto too_many;
            gp[ngp++] = (uint64_t)cv.v.u32;
            break;
        case AS_T_INT64:
            if (ngp >= 6) goto too_many;
            gp[ngp++] = (uint64_t)cv.v.i64;
            break;
        case AS_T_UINT64:
            if (ngp >= 6) goto too_many;
            gp[ngp++] = cv.v.u64;
            break;
        case AS_T_FLOAT: {
            uint32_t bits;
            if (nfp >= 4) goto too_many;
            memcpy(&bits, &cv.v.f32, sizeof bits);
            fp[nfp] = 0.0;
            memcpy(&fp[nfp], &bits, sizeof bits);
            nfp++;
            break;
        }
        case AS_T_DOUBLE:
            if (nfp >= 4) goto too_many;
            fp[nfp++] = cv.v.f64;
            break;
        default:
            ctx_exception(ctx, 0, "unsupported parameter type for a registered function");
            return -1;
        }
    }
    switch (rf->ret) {
    case AS_T_VOID:
        (void)as_tramp_call_int(rf->func, gp, fp, ngp, nfp);
        *ret = as_value_void();
        return 0;
    case AS_T_BOOL:
        *ret = as_value_bool(as_tramp_call_int(rf->func, gp, fp, ngp, nfp) != 0);
        return 0;
    case AS_T_INT:
        *ret = as_value_int((int32_t)as_tramp_call_int(rf->func, gp, fp, ngp, nfp));
        return 0;
    case AS_T_UINT:
        *ret = as_value_uint((uint32_t)as_tramp_call_int(rf->func, gp, fp, ngp, nfp));
        return 0;
    case AS_T_INT64:
        *ret = as_value_int64((int64_t)as_tramp_call_int(rf->func, gp, fp, ngp, nfp));
        return 0;
    case AS_T_UINT64:
        *ret = as_value_uint64(as_tramp_call_int(rf->func, gp, fp, ngp, nfp));
        return 0;
    case AS_T_FLOAT:
        *ret = as_value_float(as_tramp_call_float(rf->func, gp, fp, ngp, nfp));
        return 0;
    case AS_T_DOUBLE:
        *ret = as_value_double(as_tramp_call_double(rf->func, gp, fp, ngp, nfp));
        return 0;
    default:
        ctx_exception(ctx, 0, "unsupported return type for a registered function");
        return -1;
    }
too_many:
    ctx_exception(ctx, 0, "too many arguments for the call trampoline (max 6 GP, 4 FP)");
    return -1;
}

/* ------------------------------------------------------------------ */
/* Context                                                             */
/* ------------------------------------------------------------------ */

int asContext_AddRef(asIScriptContext *c)
{
    if (!c) return asINVALID_ARG;
    return ++c->refCount;
}

int asContext_Release(asIScriptContext *c)
{
    if (!c) return asINVALID_ARG;
    if (--c->refCount > 0) return c->refCount;
    free(c);
    return 0;
}

/* Register the three lifecycle edges on a fresh machine. */
static void rf_register_lifecycle(rf_machine_t *vm)
{
    rf_register_edge(vm, RF_LABEL_PREPARE, RF_LABEL_VALIDATED,
                     RF_PHASE_PENDING, RF_PHASE_VALIDATED);
    rf_register_edge(vm, RF_LABEL_EXEC_OK, RF_LABEL_COMMITTED,
                     RF_PHASE_VALIDATED, RF_PHASE_COMMITTED);
    rf_register_edge(vm, RF_LABEL_EXEC_FAIL, RF_LABEL_REJECTED,
                     RF_PHASE_VALIDATED, RF_PHASE_REJECTED);
}

int asContext_Prepare(asIScriptContext *c, asIScriptFunction *func)
{
    int32_t out_target = 0;
    rf_status_t st;
    int i;
    if (!c || !func) return asINVALID_ARG;
    if (c->state == asEXECUTION_ACTIVE) return asCONTEXT_ACTIVE;
    c->func = func;
    c->state = asEXECUTION_PREPARED;
    c->has_retval = 0;
    c->retval = as_value_void();
    c->exception[0] = '\0';
    c->exception_line = 0;
    c->exception_col = 0;
    c->exception_section[0] = '\0';
    c->abort_flag = 0;
    for (i = 0; i < AS_MAX_ARGS; i++) {
        c->argset[i] = 0;
        c->args[i] = as_value_void();
    }
    /* Initialise the reverse-flow machine: fuel = step budget + headroom
     * for the two lifecycle transitions, then step PENDING -> VALIDATED. */
    rf_init(&c->rf, (int32_t)(c->max_steps + 2));
    rf_register_lifecycle(&c->rf);
    st = rf_step(&c->rf, RF_LABEL_PREPARE, &out_target);
    if (st != RF_OK) return asERROR;
    return asSUCCESS;
}

int asContext_Execute(asIScriptContext *c)
{
    as_exec ex;
    as_value args[AS_MAX_ARGS];
    as_value ret;
    int32_t out_target = 0;
    int i, r;
    if (!c) return asERROR;
    if (!c->func || c->state != asEXECUTION_PREPARED)
        return asCONTEXT_NOT_PREPARED;
    if (c->state == asEXECUTION_ACTIVE) return asCONTEXT_ACTIVE;
    c->state = asEXECUTION_ACTIVE;
    memset(&ex, 0, sizeof ex);
    ex.ctx = c;
    ex.engine = c->engine;
    ex.module = c->func->module;
    ex.call_depth = 0;
    ex.aborted = 0;
    /* Bind prepared arguments (SetArg*) to the function parameters;
     * unset parameters default to zero. */
    for (i = 0; i < c->func->nparams && i < AS_MAX_ARGS; i++) {
        as_value zero = as_value_int(0), cv;
        if (c->argset[i]) {
            if (as_value_cast(&c->args[i], c->func->params[i], &cv) != 0) {
                c->state = asEXECUTION_EXCEPTION;
                snprintf(c->exception, sizeof c->exception,
                         "cannot convert argument %d to the parameter type", i);
                c->exception_line = 0;
                rf_step(&c->rf, RF_LABEL_EXEC_FAIL, &out_target);
                return asEXECUTION_EXCEPTION;
            }
            args[i] = cv;
        } else {
            if (as_value_cast(&zero, c->func->params[i], &cv) != 0) {
                c->state = asEXECUTION_EXCEPTION;
                snprintf(c->exception, sizeof c->exception,
                         "cannot default argument %d", i);
                c->exception_line = 0;
                rf_step(&c->rf, RF_LABEL_EXEC_FAIL, &out_target);
                return asEXECUTION_EXCEPTION;
            }
            args[i] = cv;
        }
    }
    r = as_exec_function(&ex, c->func, args, c->func->nparams, &ret);
    if (r == 0) {
        c->retval = ret;
        c->has_retval = (c->func->ret != AS_T_VOID);
        c->state = asEXECUTION_FINISHED;
        /* VALIDATED -> COMMITTED */
        rf_step(&c->rf, RF_LABEL_EXEC_OK, &out_target);
        return asEXECUTION_FINISHED;
    }
    /* Exception or abort: VALIDATED -> REJECTED */
    rf_step(&c->rf, RF_LABEL_EXEC_FAIL, &out_target);
    if (ex.aborted) {
        c->state = asEXECUTION_ABORTED;
        return asEXECUTION_ABORTED;
    }
    c->state = asEXECUTION_EXCEPTION;
    return asEXECUTION_EXCEPTION;
}

int asContext_Abort(asIScriptContext *c)
{
    if (!c) return asINVALID_ARG;
    c->abort_flag = 1;
    return asSUCCESS;
}

int asContext_Suspend(asIScriptContext *c)
{
    (void)c;
    /* A tree-walking interpreter has no resumable continuation, so
     * suspend/resume is not implemented in this subset. */
    return asNOT_SUPPORTED;
}

void *asContext_GetAddressOfArg(asIScriptContext *c, asUINT arg)
{
    if (!c || arg >= AS_MAX_ARGS) return NULL;
    return (void *)&c->args[arg].v;
}

void *asContext_GetAddressOfReturnValue(asIScriptContext *c)
{
    if (!c || !c->has_retval) return NULL;
    return (void *)&c->retval.v;
}

int asContext_SetArgObject(asIScriptContext *c, asUINT arg, void *obj)
{
    if (!c || arg >= AS_MAX_ARGS) return asINVALID_ARG;
    c->args[arg] = as_value_uint64((uint64_t)(uintptr_t)obj);
    c->argset[arg] = 1;
    return asSUCCESS;
}

int asContext_SetArgByte(asIScriptContext *c, asUINT arg, unsigned char value)
{
    if (!c || arg >= AS_MAX_ARGS) return asINVALID_ARG;
    c->args[arg] = as_value_int((int32_t)value);
    c->argset[arg] = 1;
    return asSUCCESS;
}

int asContext_SetArgWord(asIScriptContext *c, asUINT arg, unsigned short value)
{
    if (!c || arg >= AS_MAX_ARGS) return asINVALID_ARG;
    c->args[arg] = as_value_int((int32_t)value);
    c->argset[arg] = 1;
    return asSUCCESS;
}

int asContext_SetArgDWord(asIScriptContext *c, asUINT arg, asDWORD value)
{
    if (!c || arg >= AS_MAX_ARGS) return asINVALID_ARG;
    c->args[arg] = as_value_uint((uint32_t)value);
    c->argset[arg] = 1;
    return asSUCCESS;
}

int asContext_SetArgQWord(asIScriptContext *c, asUINT arg, asQWORD value)
{
    if (!c || arg >= AS_MAX_ARGS) return asINVALID_ARG;
    c->args[arg] = as_value_uint64((uint64_t)value);
    c->argset[arg] = 1;
    return asSUCCESS;
}

int asContext_SetArgFloat(asIScriptContext *c, asUINT arg, float value)
{
    if (!c || arg >= AS_MAX_ARGS) return asINVALID_ARG;
    c->args[arg] = as_value_float(value);
    c->argset[arg] = 1;
    return asSUCCESS;
}

int asContext_SetArgDouble(asIScriptContext *c, asUINT arg, double value)
{
    if (!c || arg >= AS_MAX_ARGS) return asINVALID_ARG;
    c->args[arg] = as_value_double(value);
    c->argset[arg] = 1;
    return asSUCCESS;
}

int asContext_SetArgAddress(asIScriptContext *c, asUINT arg, void *addr)
{
    if (!c || arg >= AS_MAX_ARGS) return asINVALID_ARG;
    c->args[arg] = as_value_uint64((uint64_t)(uintptr_t)addr);
    c->argset[arg] = 1;
    return asSUCCESS;
}

asDWORD asContext_GetReturnDWord(asIScriptContext *c)
{
    as_value v;
    if (!c || !c->has_retval) return 0;
    if (as_value_cast(&c->retval, AS_T_UINT, &v) != 0) return 0;
    return (asDWORD)v.v.u32;
}

asQWORD asContext_GetReturnQWord(asIScriptContext *c)
{
    as_value v;
    if (!c || !c->has_retval) return 0;
    if (as_value_cast(&c->retval, AS_T_UINT64, &v) != 0) return 0;
    return (asQWORD)v.v.u64;
}

float asContext_GetReturnFloat(asIScriptContext *c)
{
    as_value v;
    if (!c || !c->has_retval) return 0.0f;
    if (as_value_cast(&c->retval, AS_T_FLOAT, &v) != 0) return 0.0f;
    return v.v.f32;
}

double asContext_GetReturnDouble(asIScriptContext *c)
{
    as_value v;
    if (!c || !c->has_retval) return 0.0;
    if (as_value_cast(&c->retval, AS_T_DOUBLE, &v) != 0) return 0.0;
    return v.v.f64;
}

void *asContext_GetReturnObject(asIScriptContext *c)
{
    if (!c || !c->has_retval) return NULL;
    if (c->retval.kind != AS_T_UINT64) return NULL;
    return (void *)(uintptr_t)c->retval.v.u64;
}

void *asContext_GetReturnAddress(asIScriptContext *c)
{
    if (!c || !c->has_retval) return NULL;
    if (c->retval.kind != AS_T_UINT64) return NULL;
    return (void *)(uintptr_t)c->retval.v.u64;
}

int asContext_GetState(asIScriptContext *c)
{
    if (!c) return asEXECUTION_ERROR;
    return c->state;
}

int asContext_GetExceptionLineNumber(asIScriptContext *c, int *column,
                                     const char **sectionName)
{
    if (!c) return 0;
    if (column) *column = c->exception_col;
    if (sectionName) *sectionName = c->exception_section;
    return c->exception_line;
}

const char *asContext_GetExceptionString(asIScriptContext *c)
{
    if (!c) return "";
    return c->exception;
}
