// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
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
