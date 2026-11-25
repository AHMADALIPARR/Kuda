#!/bin/sh
# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
set -e
export LC_ALL=C
make clean
make ats2
make c11
make tests
./tests/test_c11_abi
echo "BUILD_OK"
