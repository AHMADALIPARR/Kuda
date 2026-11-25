# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# FILE: build.sh
#!/bin/sh
set -e
export LC_ALL=C
make clean
make ats2
make c11
make backend
make tests
./tests/test_gpu_ir
echo "OCCAM_GPU_BUILD_OK"
