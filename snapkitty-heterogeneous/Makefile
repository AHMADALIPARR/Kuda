# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# snapkitty-heterogeneous/Makefile

BUILD := build
CMAKE ?= cmake
JOBS ?= 4

.PHONY: all configure build test clean mojo

all: build

configure:
	$(CMAKE) -S . -B $(BUILD) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_SYSTEM_PROCESSOR=aarch64

build: configure
	$(CMAKE) --build $(BUILD) --parallel $(JOBS)

test: build
	ctest --test-dir $(BUILD) --output-on-failure

mojo:
	mojo build mojo/kernels/bit_pattern.mojo

clean:
	rm -rf $(BUILD)
