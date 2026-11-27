-- Copyright (C) 2026 AHMADALIPARR
-- SPDX-License-Identifier: AGPL-3.0-only
local as = require("angelscript")

print("AngelScript version:", as.get_library_version())

local engine = as.create_script_engine()
assert(engine)

local mod = engine:get_module("test", 1) -- asGM_ALWAYS_CREATE
assert(mod)

local code = [[
void main() {
    // script body
}
]]

assert(mod:add_script_section("main", code) >= 0)
assert(mod:build() >= 0)

local func = mod:get_function_by_decl("void main()")
assert(func)

local ctx = engine:create_context()
assert(ctx)

assert(ctx:prepare(func) >= 0)
assert(ctx:execute() == 0) -- asEXECUTION_FINISHED == 0

print("PASS")
