// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* test_replay.c - bridge integration checks for the C11 engine.
 * 1. A script with a loop + registered calls executes to FINISHED, the
 *    return value is correct, and rf_replay_verify accepts the trace
 *    with final phase COMMITTED.
 * 2. An infinite loop with a small fuel budget fails with
 *    asEXECUTION_EXCEPTION (fuel exhausted), final phase REJECTED, and
 *    rf_replay_verify still accepts the trace.
 * Internal test: includes as_c11.h to reach ctx->rf and ctx->max_steps.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "as_c11.h"
#include "bridge.h"

static int fails = 0;
#define CHECK(c) do { \
    if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } \
    else printf("PASS: %s\n", #c); \
} while (0)

static void Msg(const void *msg, void *param)
{
    const asSMessageInfo *m = (const asSMessageInfo *)msg;
    (void)param;
    if (m) printf("MSG [%s:%d] %s\n", m->section, m->row, m->message);
}

static int Add(int a, int b) { return a + b; }
static void PrintHello(void) { puts("Hello from pure-C registered function"); }

int main(void)
{
    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    assert(engine);
    assert(asEngine_SetMessageCallback(engine, Msg, 0, asCALL_CDECL) >= 0);
    assert(asEngine_RegisterGlobalFunction(engine, "int add(int, int)",
                                           (asFUNCTION_t)Add, asCALL_CDECL) >= 0);
    assert(asEngine_RegisterGlobalFunction(engine, "void print()",
                                           (asFUNCTION_t)PrintHello, asCALL_CDECL) >= 0);

    /* ---- test 1: loop + registered calls, replay verifies ---- */
    {
        asIScriptModule *mod = asEngine_GetModule(engine, "replay", asGM_ALWAYS_CREATE);
        asIScriptFunction *fn;
        asIScriptContext *ctx;
        rf_phase_t phase = RF_PHASE_PENDING;
        int32_t fuel = 0;
        int r;
        assert(mod);
        const char *code =
            "int sum(int n) { int s = 0; int i = 0; "
            "  while (i < n) { s = add(s, i); i = i + 1; } "
            "  return s; }"
            "void main() { int t = sum(10); print(); }";
        assert(asModule_AddScriptSection(mod, "replay", code, 0, 0) >= 0);
        assert(asModule_Build(mod) >= 0);
        fn = asModule_GetFunctionByDecl(mod, "int sum(int)");
        assert(fn);
        ctx = asEngine_CreateContext(engine);
        assert(ctx);
        assert(asContext_Prepare(ctx, fn) >= 0);
        assert(asContext_SetArgDWord(ctx, 0, 10) >= 0);
        r = asContext_Execute(ctx);
        CHECK(r == asEXECUTION_FINISHED);
        CHECK(asContext_GetReturnDWord(ctx) == 45); /* 0+1+...+9 */
        CHECK(asContext_GetState(ctx) == asEXECUTION_FINISHED);
        CHECK(rf_replay_verify(&ctx->rf, &phase, &fuel) == RF_OK);
        CHECK(phase == RF_PHASE_COMMITTED);
        CHECK(strcmp(rf_phase_name(phase), "COMMITTED") == 0);
        printf("replay trace: count=%u phase=%s fuel=%d\n",
               (unsigned)ctx->rf.trace_count, rf_phase_name(phase), (int)fuel);
        /* main() also runs print() through the same path */
        fn = asModule_GetFunctionByDecl(mod, "void main()");
        assert(fn);
        assert(asContext_Prepare(ctx, fn) >= 0);
        r = asContext_Execute(ctx);
        CHECK(r == asEXECUTION_FINISHED);
        CHECK(rf_replay_verify(&ctx->rf, &phase, &fuel) == RF_OK);
        CHECK(phase == RF_PHASE_COMMITTED);
        asContext_Release(ctx);
        asModule_Release(mod);
    }

    /* ---- test 2: fuel exhaustion on an infinite loop ---- */
    {
        asIScriptModule *mod = asEngine_GetModule(engine, "fuel", asGM_ALWAYS_CREATE);
        asIScriptFunction *fn;
        asIScriptContext *ctx;
        rf_phase_t phase = RF_PHASE_PENDING;
        int32_t fuel = 0;
        int r;
        assert(mod);
        const char *code = "void spin() { while (true) { } }";
        assert(asModule_AddScriptSection(mod, "fuel", code, 0, 0) >= 0);
        assert(asModule_Build(mod) >= 0);
        fn = asModule_GetFunctionByDecl(mod, "void spin()");
        assert(fn);
        ctx = asEngine_CreateContext(engine);
        assert(ctx);
        ctx->max_steps = 100; /* tiny budget, set before Prepare */
        assert(asContext_Prepare(ctx, fn) >= 0);
        r = asContext_Execute(ctx);
        CHECK(r == asEXECUTION_EXCEPTION);
        CHECK(asContext_GetState(ctx) == asEXECUTION_EXCEPTION);
        printf("fuel exception: %s (line %d)\n",
               asContext_GetExceptionString(ctx),
               asContext_GetExceptionLineNumber(ctx, 0, 0));
        CHECK(strstr(asContext_GetExceptionString(ctx), "fuel") != NULL);
        CHECK(rf_replay_verify(&ctx->rf, &phase, &fuel) == RF_OK);
        CHECK(phase == RF_PHASE_REJECTED);
        asContext_Release(ctx);
        asModule_Release(mod);
    }

    asEngine_Release(engine);
    if (fails) { printf("REPLAY-FAIL (%d)\n", fails); return 1; }
    puts("REPLAY-OK");
    return 0;
}
