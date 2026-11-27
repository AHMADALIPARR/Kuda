# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# Makefile - C11 AngelScript-subset engine on the reverse-flow-machine spine.
# Builds the engine as a static library (libasc11.a) and runs the tests:
#   tests/test_c11_bridge.c  (the dropped C API contract: Hello + PASS)
#   tests/test_replay.c      (bridge replay + fuel-exhaustion checks)
# Pure C11, no C++, no SDK. The bridge is compiled from
# ../../reverse-flow-machine/ffi/ (read-only; not modified).
# The x86-64 trampoline is assembled from ../asm/callx64_nasm.asm.

CC := cc
NASM := nasm
AR := ar

# -Wno-cast-function-type: the C API takes asFUNCTION_t (a generic
# void(*)() function pointer), so callers must cast concrete function
# pointers to it (the dropped test does the same for PrintHello).
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Wno-cast-function-type \
          -I. -I../add_on/clib -I../../reverse-flow-machine/ffi

BUILD := build
LIB := $(BUILD)/libasc11.a

ENGINE_SRC := as_lexer.c as_parser.c as_value.c as_interp.c as_engine.c
BRIDGE_SRC := ../../reverse-flow-machine/ffi/bridge.c
ENGINE_OBJ := $(ENGINE_SRC:.c=.o)
ENGINE_OBJ := $(addprefix $(BUILD)/,$(ENGINE_OBJ))
BRIDGE_OBJ := $(BUILD)/bridge.o
TRAMP_OBJ := $(BUILD)/callx64_sysv.o

.PHONY: all test clean

all: $(LIB) $(BUILD)/test_c11_bridge $(BUILD)/test_replay

$(BUILD)/%.o: %.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BRIDGE_OBJ): $(BRIDGE_SRC)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(TRAMP_OBJ): ../asm/callx64_nasm.asm
	@mkdir -p $(BUILD)
	$(NASM) -f elf64 $< -o $@

$(LIB): $(ENGINE_OBJ) $(BRIDGE_OBJ) $(TRAMP_OBJ)
	$(AR) rcs $@ $^

# The dropped test includes only angelscript_c.h, which does not define
# ANGELSCRIPT_VERSION / asCALL_CDECL / asGM_ALWAYS_CREATE /
# asEXECUTION_FINISHED. Force-include as_defs.h (the constants header)
# so the test compiles without edits.
$(BUILD)/test_c11_bridge: ../../tests/test_c11_bridge.c $(LIB)
	$(CC) $(CFLAGS) -include as_defs.h $< $(LIB) -o $@

$(BUILD)/test_replay: tests/test_replay.c $(LIB)
	$(CC) $(CFLAGS) -include as_defs.h $< $(LIB) -o $@

test: all
	./$(BUILD)/test_c11_bridge
	./$(BUILD)/test_replay

clean:
	rm -rf $(BUILD)
