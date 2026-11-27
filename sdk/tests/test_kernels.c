// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <stdio.h>
#include <string.h>
#include "c_kernels.h"

static int fails = 0;
#define CHECK(cond) do { \
    if (!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); fails++; } \
    else printf("PASS: %s\n", #cond); \
} while (0)

int main(void) {
    float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float y[4]; memcpy(y, b, sizeof y);

    CHECK(k_sumf(a, 4) == 10.0f);
    CHECK(k_dotf(a, b, 4) == 10.0f);
    k_axpyf(2.0f, a, y, 4);
    CHECK(y[3] == 9.0f);
    CHECK(k_popcnt64(0xFFFFFFFFFFFFFFFFULL) == 64);
    CHECK(k_popcnt64(0) == 0);
    return fails ? 1 : 0;
}
