// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/*
  AngelScript pure-C / C11 interface (angelscript_c.h)
  Compatible with AngelScript 2.39.0
  Restores the classic clib add-on ABI.
*/

#ifndef ANGELSCRIPT_C_H
#define ANGELSCRIPT_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations matching the C++ classes */
typedef struct asIScriptEngine asIScriptEngine;
typedef struct asIScriptModule asIScriptModule;
typedef struct asIScriptContext asIScriptContext;
typedef struct asIScriptGeneric asIScriptGeneric;
typedef struct asIScriptObject asIScriptObject;
typedef struct asITypeInfo asITypeInfo;
typedef struct asIScriptFunction asIScriptFunction;

/* Common types already defined in angelscript.h – re-export for pure C */
typedef unsigned int asUINT;
typedef int asINT;
typedef unsigned long asDWORD;
typedef long long asINT64;
typedef unsigned long long asQWORD;
typedef void (*asFUNCTION_t)();

/* Message callback type used by SetMessageCallback */
typedef void (*asMESSAGEFUNC_t)(const void *msg, void *param);

/* ------------------------------------------------------------------ */
/* Engine */
/* ------------------------------------------------------------------ */

asIScriptEngine *asCreateScriptEngine(asDWORD version);

int asEngine_AddRef(asIScriptEngine *e);
int asEngine_Release(asIScriptEngine *e);

int asEngine_SetMessageCallback(asIScriptEngine *e, asMESSAGEFUNC_t callback, void *obj, asDWORD callConv);
int asEngine_ClearMessageCallback(asIScriptEngine *e);

int asEngine_RegisterGlobalFunction(asIScriptEngine *e, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
int asEngine_RegisterObjectType(asIScriptEngine *e, const char *name, int byteSize, asDWORD flags);
int asEngine_RegisterObjectMethod(asIScriptEngine *e, const char *obj, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
int asEngine_RegisterObjectProperty(asIScriptEngine *e, const char *obj, const char *declaration, int byteOffset);
int asEngine_RegisterObjectBehaviour(asIScriptEngine *e, const char *obj, asDWORD behaviour, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);

asIScriptModule *asEngine_GetModule(asIScriptEngine *e, const char *module, asDWORD flag);
int asEngine_DiscardModule(asIScriptEngine *e, const char *module);

asIScriptContext *asEngine_CreateContext(asIScriptEngine *e);

const char *asEngine_GetLibraryVersion(void);
const char *asEngine_GetLibraryOptions(void);

/* ------------------------------------------------------------------ */
/* Module */
/* ------------------------------------------------------------------ */

int asModule_AddRef(asIScriptModule *m);
int asModule_Release(asIScriptModule *m);

int asModule_AddScriptSection(asIScriptModule *m, const char *name, const char *code, size_t codeLength, int lineOffset);
int asModule_Build(asIScriptModule *m);

asIScriptFunction *asModule_GetFunctionByDecl(asIScriptModule *m, const char *decl);
asIScriptFunction *asModule_GetFunctionByName(asIScriptModule *m, const char *name);
int asModule_GetFunctionCount(asIScriptModule *m);

/* ------------------------------------------------------------------ */
/* Context */
/* ------------------------------------------------------------------ */

int asContext_AddRef(asIScriptContext *c);
int asContext_Release(asIScriptContext *c);

int asContext_Prepare(asIScriptContext *c, asIScriptFunction *func);
int asContext_Execute(asIScriptContext *c);
int asContext_Abort(asIScriptContext *c);
int asContext_Suspend(asIScriptContext *c);

void *asContext_GetAddressOfArg(asIScriptContext *c, asUINT arg);
void *asContext_GetAddressOfReturnValue(asIScriptContext *c);
int asContext_SetArgObject(asIScriptContext *c, asUINT arg, void *obj);
int asContext_SetArgByte(asIScriptContext *c, asUINT arg, unsigned char value);
int asContext_SetArgWord(asIScriptContext *c, asUINT arg, unsigned short value);
int asContext_SetArgDWord(asIScriptContext *c, asUINT arg, asDWORD value);
int asContext_SetArgQWord(asIScriptContext *c, asUINT arg, asQWORD value);
int asContext_SetArgFloat(asIScriptContext *c, asUINT arg, float value);
int asContext_SetArgDouble(asIScriptContext *c, asUINT arg, double value);
int asContext_SetArgAddress(asIScriptContext *c, asUINT arg, void *addr);

asDWORD asContext_GetReturnDWord(asIScriptContext *c);
asQWORD asContext_GetReturnQWord(asIScriptContext *c);
float asContext_GetReturnFloat(asIScriptContext *c);
double asContext_GetReturnDouble(asIScriptContext *c);
void *asContext_GetReturnObject(asIScriptContext *c);
void *asContext_GetReturnAddress(asIScriptContext *c);

int asContext_GetState(asIScriptContext *c);
int asContext_GetExceptionLineNumber(asIScriptContext *c, int *column, const char **sectionName);
const char *asContext_GetExceptionString(asIScriptContext *c);

#ifdef __cplusplus
}
#endif

#endif /* ANGELSCRIPT_C_H */
