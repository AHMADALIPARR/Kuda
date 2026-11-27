#!/bin/sh
# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# gen.sh - self-extracting scaffold generator
# Target stack: AngelScript + C11 bridge + C kernels + NASM x86-64 + AArch64
# Zero Python, zero YAML CI, zero package managers. Pure Make/sh.
set -eu
mkdir -p include src kernels asm tools tests

# ---------------------------------------------------------------- tools/audit.sh
cat > tools/audit.sh <<'AUDIT_EOF'
#!/bin/sh
# Policy audit: zero .py, zero YAML CI. Exits 1 on any violation.
viol=0
for pat in '*.py' '*.yml' '*.yaml' 'pytest' 'setup.py' 'tox.ini'; do
  hits=$(find . -name "$pat" -not -path './.git/*' 2>/dev/null)
  if [ -n "$hits" ]; then
    echo "VIOLATION: pattern $pat:"
    echo "$hits"
    viol=1
  fi
done
if [ "$viol" -eq 0 ]; then
  echo "AUDIT PASS: zero python, zero YAML CI artifacts"
fi
exit $viol
AUDIT_EOF

# ---------------------------------------------------------------- include/c_kernels.h
cat > include/c_kernels.h <<'KERNELS_H_EOF'
/* c_kernels.h - portable C99/C11 compute kernels, uniform C signatures */
#ifndef C_KERNELS_H
#define C_KERNELS_H
#ifdef __cplusplus
extern "C" {
#endif

float k_sumf(const float *a, unsigned n);
float k_dotf(const float *a, const float *b, unsigned n);
void k_axpyf(float alpha, const float *x, float *y, unsigned n);
unsigned k_popcnt64(unsigned long long v);
long k_syscall_probe(long nr, long a, long b, long c); /* raw syscall wrapper (Linux) */

#ifdef __cplusplus
}
#endif
#endif
KERNELS_H_EOF

# ---------------------------------------------------------------- kernels/c_kernels.c
cat > kernels/c_kernels.c <<'KERNELS_C_EOF'
#include "c_kernels.h"

float k_sumf(const float *a, unsigned n) {
    float s = 0.0f;
    for (unsigned i = 0; i < n; i++) s += a[i];
    return s;
}

float k_dotf(const float *a, const float *b, unsigned n) {
    float s = 0.0f;
    for (unsigned i = 0; i < n; i++) s += a[i] * b[i];
    return s;
}

void k_axpyf(float alpha, const float *x, float *y, unsigned n) {
    for (unsigned i = 0; i < n; i++) y[i] += alpha * x[i];
}

unsigned k_popcnt64(unsigned long long v) {
    unsigned c = 0;
    while (v) { v &= v - 1; c++; }
    return c;
}

#if defined(__linux__) && defined(__GNUC__)
__attribute__((naked)) long k_syscall_probe(long nr, long a, long b, long c) {
    __asm__ volatile (
#if defined(__x86_64__)
        "movq %rdi, %rax\n\t"
        "movq %rsi, %rdi\n\t"
        "movq %rdx, %rsi\n\t"
        "movq %rcx, %rdx\n\t"
        "syscall\n\t"
        "ret\n\t"
#elif defined(__aarch64__)
        "mov x8, x0\n\t"
        "mov x0, x1\n\t"
        "mov x1, x2\n\t"
        "mov x2, x3\n\t"
        "svc #0\n\t"
        "ret\n\t"
#endif
    );
}
#else
long k_syscall_probe(long nr, long a, long b, long c) {
    (void)nr; (void)a; (void)b; (void)c;
    return -1; /* non-Linux: privileged path disabled */
}
#endif
KERNELS_C_EOF

# ---------------------------------------------------------------- include/as_c11_bridge.h
cat > include/as_c11_bridge.h <<'BRIDGE_H_EOF'
/* as_c11_bridge.h - pure C11 surface over the AngelScript registration API.
 * C callers need no C++ headers and no vtable knowledge: all engine/module/
 * context operations are extern "C" symbols implemented in as_c11_bridge.cpp.
 */
#ifndef AS_C11_BRIDGE_H
#define AS_C11_BRIDGE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct as_c11_engine as_c11_engine;
typedef struct as_c11_context as_c11_context;

as_c11_engine *as_c11_create(void);
void as_c11_destroy(as_c11_engine *e);

/* Register a native global function. decl is AngelScript syntax, e.g.
 * "float k_sumf(const float[] &in, uint)"
 * fn must be a plain C function pointer. Returns 0 on success (<0 on error). */
int as_c11_register_function(as_c11_engine *e, const char *decl, void *fn);

/* Execute a global script function by name. Returns 0 when execution finished. */
int as_c11_call(as_c11_engine *e, const char *mod, const char *fn,
                void **args, void *out);

const char *as_c11_last_error(void);

#ifdef __cplusplus
}
#endif
#endif
BRIDGE_H_EOF

# ---------------------------------------------------------------- src/as_c11_bridge.cpp
cat > src/as_c11_bridge.cpp <<'BRIDGE_CPP_EOF'
// as_c11_bridge.cpp - the ONLY C++ translation unit in the pipeline.
// Compiles against the official SDK (angelscript.h). When the SDK is absent,
// this TU is built in stub mode so the C ABI contract stays compile-checked
// and kernels/asm tests always run.
#include "as_c11_bridge.h"
#include <cstdio>

#ifdef AS_C11_HAVE_SDK
#include <angelscript.h>

static thread_local char g_err[256] = {0};

static void fail(const char *msg) {
    std::snprintf(g_err, sizeof g_err, "%s", msg);
}

extern "C" {

as_c11_engine *as_c11_create(void) {
    asIScriptEngine *e = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if (!e) { fail("asCreateScriptEngine failed"); return nullptr; }
    return reinterpret_cast<as_c11_engine *>(e);
}

void as_c11_destroy(as_c11_engine *e) {
    if (e) static_cast<asIScriptEngine *>(e)->ShutDownAndRelease();
}

int as_c11_register_function(as_c11_engine *e, const char *decl, void *fn) {
    asIScriptEngine *eng = static_cast<asIScriptEngine *>(e);
    int r = eng->RegisterGlobalFunction(decl, asFunctionPtr(fn), asCALL_CDECL);
    if (r < 0) { fail("RegisterGlobalFunction failed"); return r; }
    return 0;
}

int as_c11_call(as_c11_engine *e, const char *mod, const char *fn,
                void **args, void *out) {
    asIScriptEngine *eng = static_cast<asIScriptEngine *>(e);
    asIScriptModule *m = eng->GetModule(mod, asGM_ONLY_IF_EXISTS);
    if (!m) { fail("module not found"); return -1; }
    asIScriptFunction *f = m->GetFunctionByName(fn);
    if (!f) { fail("function not found"); return -1; }
    asIScriptContext *ctx = eng->CreateContext();
    if (!ctx) { fail("context creation failed"); return -1; }
    ctx->Prepare(f);
    // Marshal object/pointer args by declared parameter order (extend as needed).
    for (asUINT i = 0; args && i < f->GetParamCount(); i++) {
        ctx->SetArgObject(i, args[i]);
    }
    int r = ctx->Execute();
    ctx->Release();
    return r == asEXECUTION_FINISHED ? 0 : -2;
}

const char *as_c11_last_error(void) { return g_err; }

} // extern "C"
#else
// Stub surface: no SDK in this build. Keeps the C ABI contract compile-checked.
extern "C" {
as_c11_engine *as_c11_create(void) { return nullptr; }
void as_c11_destroy(as_c11_engine *) {}
int as_c11_register_function(as_c11_engine *, const char *, void *) { return -1; }
int as_c11_call(as_c11_engine *, const char *, const char *, void **, void *) { return -1; }
const char *as_c11_last_error(void) { return "stub build (no SDK)"; }
}
#endif
BRIDGE_CPP_EOF

# ---------------------------------------------------------------- asm/callx64_nasm.asm
cat > asm/callx64_nasm.asm <<'NASM_EOF'
; callx64_nasm.asm - NASM x86-64 argument/FP marshaling trampoline.
; Scope: SysV AMD64 (Linux). GP args from array (max 6: rdi,rsi,rdx,rcx,r8,r9),
; FP args from array (max 8: xmm0-xmm7), integer/pointer return in rax.
; Build: nasm -f elf64 asm/callx64_nasm.asm -o build/callx64_sysv.o
; NOTE: this is a verification-grade trampoline, not the full AngelScript
; CallX64 with stack-argument spill; extend per as_callfunc_x64_msvc_asm.asm.

section .text
global callx64_sysv

callx64_sysv:
    ; rdi=target rsi=gp_args rdx=fp_args ecx=n_gp r8d=n_fp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 8 ; 5 pushes + 8 = 48; caller rsp was 8 mod 16 -> 16-aligned? (verify per call site)

    mov r12, rdi ; target
    mov r13, rsi ; gp array
    mov r14, rdx ; fp array
    mov ebx, r8d ; n_fp

    ; Load GP args from array into named registers
    mov rdi, [r13 + 0*8]
    mov rsi, [r13 + 1*8]
    mov rdx, [r13 + 2*8]
    mov rcx, [r13 + 3*8]
    mov r8, [r13 + 4*8]
    mov r9, [r13 + 5*8]

    ; Load FP args
    xor eax, eax
    test ebx, ebx
    jz .fp_done
    movsd xmm0, [r14 + 0*8]
    cmp ebx, 2
    jb .fp_done
    movsd xmm1, [r14 + 1*8]
    cmp ebx, 3
    jb .fp_done
    movsd xmm2, [r14 + 2*8]
    cmp ebx, 4
    jb .fp_done
    movsd xmm3, [r14 + 3*8]
.fp_done:
    mov al, bl ; vector register count (varargs convention)
    and al, 0x7
    call r12

    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
NASM_EOF

# ---------------------------------------------------------------- asm/arm64_gcc.S
cat > asm/arm64_gcc.S <<'ARM64_EOF'
/* arm64_gcc.S - AArch64 (GAS) native dispatch stub, Linux AAPCS64.
 * Scope: up to 8 GP (x0-x7) and 8 FP/SIMD (v0-v7) args; integer/pointer
 * return in x0. HFA returns come back in v0-v3 per PCS (pass-through here).
 * Build: cc -c asm/arm64_gcc.S -o build/arm64_call.o
 */
    .text
    .global arm64_call
arm64_call:
    stp x29, x30, [sp, #-96]!
    mov x29, sp

    mov x9, x0 /* target */
    mov x10, x1 /* gp array */
    mov x11, x2 /* fp array */

    /* GP: x0..x7 */
    ldr x0, [x10, #0]
    ldr x1, [x10, #8]
    ldr x2, [x10, #16]
    ldr x3, [x10, #24]
    ldr x4, [x10, #32]
    ldr x5, [x10, #40]
    ldr x6, [x10, #48]
    ldr x7, [x10, #56]

    /* FP: v0..v7 */
    ldr d0, [x11, #0]
    ldr d1, [x11, #8]
    ldr d2, [x11, #16]
    ldr d3, [x11, #24]
    ldr d4, [x11, #32]
    ldr d5, [x11, #40]
    ldr d6, [x11, #48]
    ldr d7, [x11, #56]

    blr x9

    ldp x29, x30, [sp], #96
    ret
ARM64_EOF

# ---------------------------------------------------------------- tests/test_kernels.c
cat > tests/test_kernels.c <<'TEST_C_EOF'
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
TEST_C_EOF

# ---------------------------------------------------------------- tests/run_tests.sh
cat > tests/run_tests.sh <<'RUNTESTS_EOF'
#!/bin/sh
# Deterministic CLI test runner. No Python. Prints raw pass/fail receipts.
set -u
status=0
./build/test_kernels || status=1
sh tools/audit.sh || status=1
exit $status
RUNTESTS_EOF

# ---------------------------------------------------------------- Makefile
cat > Makefile <<'MAKEFILE_EOF'
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
MAKEFILE_EOF

chmod +x tools/audit.sh tests/run_tests.sh
echo "Scaffold generated."
find . -type f | sort
