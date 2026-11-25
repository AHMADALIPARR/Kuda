# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# benchmark.R — statistical validation only, never in the GPU execution path
df <- read.csv("gpu_timings.csv") # columns: backend, elapsed_ms, bytes
df$gb_s <- (df$bytes / 1e9) / (df$elapsed_ms / 1e3)
t.test(gb_s ~ backend, data = df)
