#!/bin/sh
# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# Deterministic CLI test runner. No Python. Prints raw pass/fail receipts.
set -u
status=0
./build/test_kernels || status=1
sh tools/audit.sh || status=1
exit $status
