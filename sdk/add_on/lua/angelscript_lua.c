// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/*
  AngelScript pure-C11 → Lua adapter implementation
  Uses only angelscript_c.h – no C++ vtables exposed to Lua.
*/

#include "angelscript_c.h"
#include "angelscript_lua.h"

#include <string.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Internal userdata tags */
/* ------------------------------------------------------------------ */

#define AS_ENGINE_MT "AngelScript.Engine"
#define AS_MODULE_MT "AngelScript.Module"
#define AS_CONTEXT_MT "AngelScript.Context"

/* ------------------------------------------------------------------ */
/* Helpers */
/* ------------------------------------------------------------------ */

static asIScriptEngine *check_engine(lua_State *L, int idx)
{
    return *(asIScriptEngine **)luaL_checkudata(L, idx, AS_ENGINE_MT);
}

static asIScriptModule *check_module(lua_State *L, int idx)
{
    return *(asIScriptModule **)luaL_checkudata(L, idx, AS_MODULE_MT);
}

static asIScriptContext *check_context(lua_State *L, int idx)
{
    return *(asIScriptContext **)luaL_checkudata(L, idx, AS_CONTEXT_MT);
}

static void push_engine(lua_State *L, asIScriptEngine *e)
{
    asIScriptEngine **ud = (asIScriptEngine **)lua_newuserdata(L, sizeof(asIScriptEngine *));
    *ud = e;
    luaL_getmetatable(L, AS_ENGINE_MT);
    lua_setmetatable(L, -2);
}

static void push_module(lua_State *L, asIScriptModule *m)
{
    asIScriptModule **ud = (asIScriptModule **)lua_newuserdata(L, sizeof(asIScriptModule *));
    *ud = m;
    luaL_getmetatable(L, AS_MODULE_MT);
    lua_setmetatable(L, -2);
}

static void push_context(lua_State *L, asIScriptContext *c)
{
    asIScriptContext **ud = (asIScriptContext **)lua_newuserdata(L, sizeof(asIScriptContext *));
    *ud = c;
    luaL_getmetatable(L, AS_CONTEXT_MT);
    lua_setmetatable(L, -2);
}

/* ------------------------------------------------------------------ */
/* Engine methods */
/* ------------------------------------------------------------------ */

static int engine_gc(lua_State *L)
{
    asIScriptEngine *e = check_engine(L, 1);
    if (e) {
        asEngine_Release(e);
        *(asIScriptEngine **)lua_touserdata(L, 1) = NULL;
    }
    return 0;
}

static int engine_register_global_function(lua_State *L)
{
    asIScriptEngine *e = check_engine(L, 1);
    const char *decl = luaL_checkstring(L, 2);
    /* For the minimal adapter we only support registering a Lua function
       via a thin C trampoline. Real production code would store the Lua
       reference and call it from a generic asCALL_CDECL trampoline. */
    luaL_error(L, "register_global_function: full trampoline not implemented in this minimal adapter");
    return 0;
}

static int engine_get_module(lua_State *L)
{
    asIScriptEngine *e = check_engine(L, 1);
    const char *name = luaL_checkstring(L, 2);
    asDWORD flag = (asDWORD)luaL_optinteger(L, 3, 1); /* asGM_ALWAYS_CREATE == 1 */

    asIScriptModule *m = asEngine_GetModule(e, name, flag);
    if (!m) {
        lua_pushnil(L);
        return 1;
    }
    push_module(L, m);
    return 1;
}

static int engine_create_context(lua_State *L)
{
    asIScriptEngine *e = check_engine(L, 1);
    asIScriptContext *c = asEngine_CreateContext(e);
    if (!c) {
        lua_pushnil(L);
        return 1;
    }
    push_context(L, c);
    return 1;
}

static int engine_get_library_version(lua_State *L)
{
    lua_pushstring(L, asEngine_GetLibraryVersion());
    return 1;
}

static const luaL_Reg engine_methods[] = {
    {"get_module", engine_get_module},
    {"create_context", engine_create_context},
    {"get_library_version", engine_get_library_version},
    {"__gc", engine_gc},
    {NULL, NULL}
};

/* ------------------------------------------------------------------ */
/* Module methods */
/* ------------------------------------------------------------------ */

static int module_gc(lua_State *L)
{
    asIScriptModule *m = check_module(L, 1);
    if (m) {
        asModule_Release(m);
        *(asIScriptModule **)lua_touserdata(L, 1) = NULL;
    }
    return 0;
}

static int module_add_script_section(lua_State *L)
{
    asIScriptModule *m = check_module(L, 1);
    const char *name = luaL_checkstring(L, 2);
    size_t len = 0;
    const char *code = luaL_checklstring(L, 3, &len);
    int lineOffset = (int)luaL_optinteger(L, 4, 0);

    int r = asModule_AddScriptSection(m, name, code, len, lineOffset);
    lua_pushinteger(L, r);
    return 1;
}

static int module_build(lua_State *L)
{
    asIScriptModule *m = check_module(L, 1);
    int r = asModule_Build(m);
    lua_pushinteger(L, r);
    return 1;
}

static int module_get_function_by_decl(lua_State *L)
{
    asIScriptModule *m = check_module(L, 1);
    const char *decl = luaL_checkstring(L, 2);
    asIScriptFunction *f = asModule_GetFunctionByDecl(m, decl);
    if (!f) {
        lua_pushnil(L);
        return 1;
    }
    /* For the minimal adapter we simply return the raw pointer as lightuserdata.
       A fuller binding would wrap it in its own metatable. */
    lua_pushlightuserdata(L, f);
    return 1;
}

static const luaL_Reg module_methods[] = {
    {"add_script_section", module_add_script_section},
    {"build", module_build},
    {"get_function_by_decl", module_get_function_by_decl},
    {"__gc", module_gc},
    {NULL, NULL}
};

/* ------------------------------------------------------------------ */
/* Context methods */
/* ------------------------------------------------------------------ */

static int context_gc(lua_State *L)
{
    asIScriptContext *c = check_context(L, 1);
    if (c) {
        asContext_Release(c);
        *(asIScriptContext **)lua_touserdata(L, 1) = NULL;
    }
    return 0;
}

static int context_prepare(lua_State *L)
{
    asIScriptContext *c = check_context(L, 1);
    asIScriptFunction *f = (asIScriptFunction *)lua_touserdata(L, 2);
    if (!f) return luaL_error(L, "expected function lightuserdata");

    int r = asContext_Prepare(c, f);
    lua_pushinteger(L, r);
    return 1;
}

static int context_execute(lua_State *L)
{
    asIScriptContext *c = check_context(L, 1);
    int r = asContext_Execute(c);
    lua_pushinteger(L, r);
    return 1;
}

static int context_get_return_dword(lua_State *L)
{
    asIScriptContext *c = check_context(L, 1);
    lua_pushinteger(L, (lua_Integer)asContext_GetReturnDWord(c));
    return 1;
}

static const luaL_Reg context_methods[] = {
    {"prepare", context_prepare},
    {"execute", context_execute},
    {"get_return_dword", context_get_return_dword},
    {"__gc", context_gc},
    {NULL, NULL}
};

/* ------------------------------------------------------------------ */
/* Module-level functions */
/* ------------------------------------------------------------------ */

static int l_create_script_engine(lua_State *L)
{
    asDWORD version = (asDWORD)luaL_optinteger(L, 1, 23900); /* ANGELSCRIPT_VERSION */
    asIScriptEngine *e = asCreateScriptEngine(version);
    if (!e) {
        lua_pushnil(L);
        return 1;
    }
    push_engine(L, e);
    return 1;
}

static int l_get_library_version(lua_State *L)
{
    lua_pushstring(L, asEngine_GetLibraryVersion());
    return 1;
}

static const luaL_Reg module_funcs[] = {
    {"create_script_engine", l_create_script_engine},
    {"get_library_version", l_get_library_version},
    {NULL, NULL}
};

/* ------------------------------------------------------------------ */
/* Module open */
/* ------------------------------------------------------------------ */

int luaopen_angelscript(lua_State *L)
{
    /* Engine metatable */
    luaL_newmetatable(L, AS_ENGINE_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, engine_methods, 0);
    lua_pop(L, 1);

    /* Module metatable */
    luaL_newmetatable(L, AS_MODULE_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, module_methods, 0);
    lua_pop(L, 1);

    /* Context metatable */
    luaL_newmetatable(L, AS_CONTEXT_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, context_methods, 0);
    lua_pop(L, 1);

    /* Module table */
    luaL_newlib(L, module_funcs);
    return 1;
}

/* Optional helpers requested by the header */
void as_lua_push_engine(lua_State *L, asIScriptEngine *engine)
{
    push_engine(L, engine);
}

asIScriptEngine *as_lua_check_engine(lua_State *L, int idx)
{
    return check_engine(L, idx);
}
