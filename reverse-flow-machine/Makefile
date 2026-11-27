# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O2
GHC     ?= ghc
ICK     ?= ick

BUILD_DIR = build

TESTS = \
	$(BUILD_DIR)/test_determinism \
	$(BUILD_DIR)/test_termination \
	$(BUILD_DIR)/test_persistence \
	$(BUILD_DIR)/test_replay

.PHONY: all test check-haskell check-intercal clean

all: $(TESTS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/bridge.o: ffi/bridge.c ffi/bridge.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ffi/bridge.c -o $(BUILD_DIR)/bridge.o

$(BUILD_DIR)/test_determinism: tests/determinism/test_determinism.c $(BUILD_DIR)/bridge.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_termination: tests/termination/test_termination.c $(BUILD_DIR)/bridge.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_persistence: tests/persistence/test_persistence.c $(BUILD_DIR)/bridge.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_replay: tests/replay/test_replay.c $(BUILD_DIR)/bridge.o
	$(CC) $(CFLAGS) $^ -o $@

test: $(TESTS)
	./$(BUILD_DIR)/test_determinism
	./$(BUILD_DIR)/test_termination
	./$(BUILD_DIR)/test_persistence
	./$(BUILD_DIR)/test_replay

check-haskell:
	$(GHC) -fno-code -Wall haskell/Types.hs
	$(GHC) -fno-code -Wall haskell/Invariants.hs
	$(GHC) -fno-code -Wall haskell/Transition.hs
	$(GHC) -fno-code -Wall haskell/Compiler.hs

check-intercal:
	$(ICK) -c intercal/dispatch.i
	$(ICK) -c intercal/reverse.i
	$(ICK) -c intercal/termination.i

clean:
	rm -rf $(BUILD_DIR) *.hi *.o haskell/*.hi haskell/*.o
