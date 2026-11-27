# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# Makefile - pure CLI build. Recipe prefix '>' avoids tab/space issues.
.RECIPEPREFIX = >
CC := cc
CXX := c++
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Iinclude
CXXFLAGS:= -std=c++14 -Wall -Wextra -Iinclude
NASM := nasm
SDK := $(wildcard angelscript/sdk/angelscript/include/angelscript.h)
MACH := $(shell cc -dumpmachine 2>/dev/null)

BUILD := build
OBJ := $(BUILD)

.PHONY: all audit bridge kernels asm test clean
all: kernels bridge asm test

audit:
>sh tools/audit.sh

$(OBJ)/as_c11_bridge.o: src/as_c11_bridge.cpp include/as_c11_bridge.h
>@mkdir -p $(BUILD)
>@if [ -n "$(SDK)" ]; then \
> $(CXX) $(CXXFLAGS) -Iangelscript/sdk/angelscript/include -DAS_C11_HAVE_SDK -c src/as_c11_bridge.cpp -o $@; \
> echo "BRIDGE: built with SDK"; \
>else \
> $(CXX) $(CXXFLAGS) -c src/as_c11_bridge.cpp -o $@; \
> echo "BRIDGE: stub build (SDK not found at angelscript/sdk/...)"; \
>fi
bridge: $(OBJ)/as_c11_bridge.o

$(OBJ)/c_kernels.o: kernels/c_kernels.c include/c_kernels.h
>@mkdir -p $(BUILD)
>$(CC) $(CFLAGS) -c kernels/c_kernels.c -o $@
$(OBJ)/test_kernels.o: tests/test_kernels.c include/c_kernels.h
>@mkdir -p $(BUILD)
>$(CC) $(CFLAGS) -c tests/test_kernels.c -o $@
kernels: $(OBJ)/c_kernels.o $(OBJ)/test_kernels.o

$(BUILD)/test_kernels: $(OBJ)/c_kernels.o $(OBJ)/test_kernels.o
>$(CC) $(OBJ)/c_kernels.o $(OBJ)/test_kernels.o -o $@

asm:
>@mkdir -p $(BUILD)
>@case "$(MACH)" in \
> x86_64*|amd64*) $(NASM) -f elf64 asm/callx64_nasm.asm -o $(BUILD)/callx64_sysv.o && echo "ASM: NASM x86-64 OK" ;; \
> aarch64*) $(CC) -c asm/arm64_gcc.S -o $(BUILD)/arm64_call.o && echo "ASM: AArch64 GAS OK" ;; \
> *) echo "ASM: host $(MACH) not targeted; skipping" ;; \
>esac

test: $(BUILD)/test_kernels
>sh tests/run_tests.sh

clean:
>rm -rf $(BUILD)
