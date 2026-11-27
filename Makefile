# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
CC ?= gcc
CXX ?= g++
AS ?= as
AR ?= ar
NASM ?= nasm
ARCH ?= $(shell uname -m)
SYS ?= $(shell uname -s)

CFLAGS ?= -std=c11 -fno-strict-aliasing -Wall -Wextra -Werror -O2
CXXFLAGS ?= -std=c++11 -fno-strict-aliasing -Wall -Wextra -Werror -O2

AS_DIR := sdk/angelscript/source
CLIB_DIR := sdk/add_on/clib
INC := -I sdk/angelscript/include -I sdk/add_on/clib

AS_SRC := $(wildcard $(AS_DIR)/*.cpp)
CLIB_SRC := $(wildcard $(CLIB_DIR)/*.cpp)

AS_OBJ := $(AS_SRC:.cpp=.o)
CLIB_OBJ := $(CLIB_SRC:.cpp=.o)

# Architecture-specific call-function backend
BACKEND_OBJ :=
BACKEND_RULE :=

ifeq ($(ARCH),x86_64)
  ifeq ($(SYS),Linux)
    BACKEND_OBJ := $(AS_DIR)/as_callfunc_x64_gcc.o
    BACKEND_RULE := $(AS_DIR)/as_callfunc_x64_gcc.o: $(AS_DIR)/as_callfunc_x64_gcc.S
  else ifneq (,$(findstring MINGW,$(SYS))$(findstring MSYS,$(SYS)))
    $(error Windows/MSVC build: use ml64 on as_callfunc_x64_msvc_asm.asm, not NASM)
  endif
else ifeq ($(ARCH),aarch64)
  BACKEND_OBJ := $(AS_DIR)/as_callfunc_arm64_gcc.o
  BACKEND_RULE := $(AS_DIR)/as_callfunc_arm64_gcc.o: $(AS_DIR)/as_callfunc_arm64_gcc.S
else
  $(error Unsupported ARCH=$(ARCH): falling back to AS_MAX_PORTABILITY not configured)
endif

$(BACKEND_RULE)
	$(CC) -c -o $@ $<

.PHONY: all clean test

all: libangelscript.a libangelscript_c.so test_harness

libangelscript.a: $(AS_OBJ)
	$(AR) rcs $@ $^

libangelscript_c.so: $(CLIB_OBJ) libangelscript.a $(BACKEND_OBJ)
	$(CXX) -shared -o $@ $(CLIB_OBJ) libangelscript.a $(BACKEND_OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<

test_harness: test_harness.c libangelscript_c.so
	$(CC) $(CFLAGS) $(INC) -o $@ test_harness.c \
	      -L. -langelscript_c -Wl,-rpath,'$$ORIGIN'

test: test_harness
	./test_harness

clean:
	rm -f $(AS_OBJ) $(CLIB_OBJ) $(BACKEND_OBJ) \
	      *.a *.so test_harness

CLIB_DIR = sdk/add_on/clib
CXXFLAGS += -I sdk/angelscript/include -I $(CLIB_DIR)
CFLAGS += -I sdk/angelscript/include -I $(CLIB_DIR)

libangelscript_c.a: $(CLIB_DIR)/as_c.o libangelscript.a
	ar rcs $@ $(CLIB_DIR)/as_c.o

$(CLIB_DIR)/as_c.o: $(CLIB_DIR)/as_c.cpp $(CLIB_DIR)/angelscript_c.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_c11: tests/test_c11_bridge.c libangelscript_c.a
	$(CC) $(CFLAGS) -o $@ $< -L. -langelscript_c -langelscript -lstdc++
	./$@ && echo "PASS tests/test_c11_bridge.c" || echo "FAIL tests/test_c11_bridge.c"

LUA_INC ?= /usr/include/lua5.4
LUA_LIB ?= -llua5.4

lua_adapter: sdk/add_on/lua/angelscript_lua.o libangelscript_c.a
	$(CC) -shared -fPIC -o angelscript.so sdk/add_on/lua/angelscript_lua.o \
	      -L. -langelscript_c -langelscript -lstdc++ $(LUA_LIB)

sdk/add_on/lua/angelscript_lua.o: sdk/add_on/lua/angelscript_lua.c \
                                  sdk/add_on/clib/angelscript_c.h \
                                  sdk/add_on/lua/angelscript_lua.h
	$(CC) -std=c11 -fPIC -Wall -Wextra -I sdk/angelscript/include \
	      -I sdk/add_on/clib -I sdk/add_on/lua -I $(LUA_INC) \
	      -c $< -o $@
