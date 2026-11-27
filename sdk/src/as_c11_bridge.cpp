// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// as_c11_bridge.cpp - the ONLY C++ translation unit in the pipeline.
// Compiles against the official SDK (angelscript.h). When the SDK is absent,
// this TU is built in stub mode so the C ABI contract stays compile-checked
// and kernels/asm tests always run.
#include "as_c11_bridge.h"
#include <cstdio>

#ifdef AS_C11_HAVE_SDK
#include <angelscript.h>

static thread_local char g_err[256] = {0};

static void fail(const char *msg) {
    std::snprintf(g_err, sizeof g_err, "%s", msg);
}

extern "C" {

as_c11_engine *as_c11_create(void) {
    asIScriptEngine *e = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if (!e) { fail("asCreateScriptEngine failed"); return nullptr; }
    return reinterpret_cast<as_c11_engine *>(e);
}

void as_c11_destroy(as_c11_engine *e) {
    if (e) static_cast<asIScriptEngine *>(e)->ShutDownAndRelease();
}

int as_c11_register_function(as_c11_engine *e, const char *decl, void *fn) {
    asIScriptEngine *eng = static_cast<asIScriptEngine *>(e);
    int r = eng->RegisterGlobalFunction(decl, asFunctionPtr(fn), asCALL_CDECL);
    if (r < 0) { fail("RegisterGlobalFunction failed"); return r; }
    return 0;
}

int as_c11_call(as_c11_engine *e, const char *mod, const char *fn,
                void **args, void *out) {
    asIScriptEngine *eng = static_cast<asIScriptEngine *>(e);
    asIScriptModule *m = eng->GetModule(mod, asGM_ONLY_IF_EXISTS);
    if (!m) { fail("module not found"); return -1; }
    asIScriptFunction *f = m->GetFunctionByName(fn);
    if (!f) { fail("function not found"); return -1; }
    asIScriptContext *ctx = eng->CreateContext();
    if (!ctx) { fail("context creation failed"); return -1; }
    ctx->Prepare(f);
    // Marshal object/pointer args by declared parameter order (extend as needed).
    for (asUINT i = 0; args && i < f->GetParamCount(); i++) {
        ctx->SetArgObject(i, args[i]);
    }
    int r = ctx->Execute();
    ctx->Release();
    return r == asEXECUTION_FINISHED ? 0 : -2;
}

const char *as_c11_last_error(void) { return g_err; }

} // extern "C"
#else
// Stub surface: no SDK in this build. Keeps the C ABI contract compile-checked.
extern "C" {
as_c11_engine *as_c11_create(void) { return nullptr; }
void as_c11_destroy(as_c11_engine *) {}
int as_c11_register_function(as_c11_engine *, const char *, void *) { return -1; }
int as_c11_call(as_c11_engine *, const char *, const char *, void **, void *) { return -1; }
const char *as_c11_last_error(void) { return "stub build (no SDK)"; }
}
#endif
