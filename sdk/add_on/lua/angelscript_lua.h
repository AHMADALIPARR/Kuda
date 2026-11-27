// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/*
  AngelScript pure-C11 → Lua adapter
  Header for the Lua binding module.
  Compatible with AngelScript 2.39.0 + the clib bridge.
*/

#ifndef ANGELSCRIPT_LUA_H
#define ANGELSCRIPT_LUA_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Module entry point – call from Lua with require("angelscript") */
int luaopen_angelscript(lua_State *L);

/* Optional: push an already-created engine as a full userdata */
void as_lua_push_engine(lua_State *L, asIScriptEngine *engine);

/* Optional: retrieve an engine from a userdata at the given stack index */
asIScriptEngine *as_lua_check_engine(lua_State *L, int idx);

#ifdef __cplusplus
}
#endif

#endif /* ANGELSCRIPT_LUA_H */
