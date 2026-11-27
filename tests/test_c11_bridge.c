// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <stdio.h>
#include <assert.h>
#include "angelscript_c.h"

static void MessageCallback(const void *msg, void *param)
{
    (void)param;
    /* In a real harness the asSMessageInfo structure would be inspected. */
    (void)msg;
}

static void PrintHello(void)
{
    puts("Hello from pure-C registered function");
}

int main(void)
{
    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    assert(engine);

    int r = asEngine_SetMessageCallback(engine, MessageCallback, 0, asCALL_CDECL);
    assert(r >= 0);

    r = asEngine_RegisterGlobalFunction(engine, "void print()", (asFUNCTION_t)PrintHello, asCALL_CDECL);
    assert(r >= 0);

    asIScriptModule *mod = asEngine_GetModule(engine, "test", asGM_ALWAYS_CREATE);
    assert(mod);

    const char *code = "void main() { print(); }";
    r = asModule_AddScriptSection(mod, "test", code, 0, 0);
    assert(r >= 0);

    r = asModule_Build(mod);
    assert(r >= 0);

    asIScriptFunction *func = asModule_GetFunctionByDecl(mod, "void main()");
    assert(func);

    asIScriptContext *ctx = asEngine_CreateContext(engine);
    assert(ctx);

    r = asContext_Prepare(ctx, func);
    assert(r >= 0);

    r = asContext_Execute(ctx);
    assert(r == asEXECUTION_FINISHED);

    asContext_Release(ctx);
    asModule_Release(mod);
    asEngine_Release(engine);

    puts("PASS");
    return 0;
}
