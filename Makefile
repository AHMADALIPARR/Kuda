# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# monorepo/Makefile — top-level build orchestrator.
# Builds every package directory that ships its own Makefile, in sorted
# order. No source is generated here; this file only drives the builds.
# Usage: make | make test | make check | make clean
SHELL := /bin/bash
ROOT := $(CURDIR)

# Package dirs: any top-level dir with a Makefile or a CMakeLists.txt.
PKGS := $(sort $(dir $(wildcard */Makefile)))
PKG_DIRS := $(patsubst %/,%,$(PKGS))
CMAKE_PKGS := $(sort $(dir $(wildcard */CMakeLists.txt)))
CMAKE_DIRS := $(patsubst %/,%,$(CMAKE_PKGS))

.PHONY: all test check clean list

all: check
	@for d in $(PKG_DIRS); do \
	  echo "[monorepo] build $$d"; \
	  $(MAKE) -C "$$d" || exit 1; \
	done
	@for d in $(CMAKE_DIRS); do \
	  echo "[monorepo] cmake build $$d"; \
	  cmake -S "$$d" -B "$$d/build" -DCMAKE_BUILD_TYPE=Release >/dev/null && \
	  cmake --build "$$d/build" || exit 1; \
	done
	@echo "[monorepo] all packages built"

test: check
	@for d in $(PKG_DIRS); do \
	  if grep -q '^test:' "$$d/Makefile"; then \
	    echo "[monorepo] test $$d"; \
	    $(MAKE) -C "$$d" test || exit 1; \
	  fi; \
	done
	@for d in $(CMAKE_DIRS); do \
	  echo "[monorepo] ctest $$d"; \
	  ctest --test-dir "$$d/build" --output-on-failure || exit 1; \
	done
	@echo "[monorepo] all package tests done"

# Fail closed: zero Python in the tree, AGPLv3 LICENSE present.
check:
	@if find "$(ROOT)" -name '*.py' | grep -q .; then \
	  echo "[monorepo] FAIL: .py files present in the tree" >&2; exit 1; fi
	@if grep -rnI 'py''thon' "$(ROOT)" --exclude-dir=.git | grep -q .; then \
	  echo "[monorepo] FAIL: Python references present in the tree" >&2; exit 1; fi
	@test -f "$(ROOT)/LICENSE" || { echo "[monorepo] FAIL: LICENSE missing" >&2; exit 1; }
	@grep -q 'GNU AFFERO GENERAL PUBLIC LICENSE' "$(ROOT)/LICENSE" || \
	  { echo "[monorepo] FAIL: LICENSE is not AGPLv3" >&2; exit 1; }
	@echo "[monorepo] check OK: zero Python, AGPLv3 LICENSE present"

clean:
	@for d in $(PKG_DIRS); do \
	  $(MAKE) -C "$$d" clean 2>/dev/null || true; \
	done
	@for d in $(CMAKE_DIRS); do \
	  rm -rf "$$d/build"; \
	done
	@echo "[monorepo] clean done"

list:
	@echo "packages with a Makefile:"
	@for d in $(PKG_DIRS); do echo "  $$d"; done
	@echo "packages with a CMakeLists.txt:"
	@for d in $(CMAKE_DIRS); do echo "  $$d"; done
	@echo "directories without a build file (listed, not built):"
	@for d in */; do \
	  [ -f "$$d/Makefile" ] || [ -f "$$d/CMakeLists.txt" ] || echo "  $$d"; \
	done
