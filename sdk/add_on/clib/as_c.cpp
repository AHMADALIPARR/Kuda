// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/*
  AngelScript pure-C / C11 wrapper implementation (as_c.cpp)
  Compatible with AngelScript 2.39.0
*/

#include "../../angelscript/include/angelscript.h"
#include "angelscript_c.h"

/* ------------------------------------------------------------------ */
/* Engine */
/* ------------------------------------------------------------------ */

extern "C" {

AS_API asIScriptEngine *asCreateScriptEngine(asDWORD version)
{
    return ::asCreateScriptEngine(version);
}

AS_API int asEngine_AddRef(asIScriptEngine *e)
{
    return e ? e->AddRef() : 0;
}

AS_API int asEngine_Release(asIScriptEngine *e)
{
    return e ? e->Release() : 0;
}

AS_API int asEngine_SetMessageCallback(asIScriptEngine *e, asMESSAGEFUNC_t callback, void *obj, asDWORD callConv)
{
    if (!e) return asINVALID_ARG;
    return e->SetMessageCallback(asFUNCTION(callback), obj, callConv);
}

AS_API int asEngine_ClearMessageCallback(asIScriptEngine *e)
{
    return e ? e->ClearMessageCallback() : asINVALID_ARG;
}

AS_API int asEngine_RegisterGlobalFunction(asIScriptEngine *e, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv)
{
    if (!e) return asINVALID_ARG;
    return e->RegisterGlobalFunction(declaration, asFUNCTION(funcPointer), callConv);
}

AS_API int asEngine_RegisterObjectType(asIScriptEngine *e, const char *name, int byteSize, asDWORD flags)
{
    if (!e) return asINVALID_ARG;
    return e->RegisterObjectType(name, byteSize, flags);
}

AS_API int asEngine_RegisterObjectMethod(asIScriptEngine *e, const char *obj, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv)
{
    if (!e) return asINVALID_ARG;
    return e->RegisterObjectMethod(obj, declaration, asFUNCTION(funcPointer), callConv);
}

AS_API int asEngine_RegisterObjectProperty(asIScriptEngine *e, const char *obj, const char *declaration, int byteOffset)
{
    if (!e) return asINVALID_ARG;
    return e->RegisterObjectProperty(obj, declaration, byteOffset);
}

AS_API int asEngine_RegisterObjectBehaviour(asIScriptEngine *e, const char *obj, asDWORD behaviour, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv)
{
    if (!e) return asINVALID_ARG;
    return e->RegisterObjectBehaviour(obj, behaviour, declaration, asFUNCTION(funcPointer), callConv);
}

AS_API asIScriptModule *asEngine_GetModule(asIScriptEngine *e, const char *module, asDWORD flag)
{
    return e ? e->GetModule(module, flag) : 0;
}

AS_API int asEngine_DiscardModule(asIScriptEngine *e, const char *module)
{
    return e ? e->DiscardModule(module) : asINVALID_ARG;
}

AS_API asIScriptContext *asEngine_CreateContext(asIScriptEngine *e)
{
    return e ? e->CreateContext() : 0;
}

AS_API const char *asEngine_GetLibraryVersion(void)
{
    return ::asGetLibraryVersion();
}

AS_API const char *asEngine_GetLibraryOptions(void)
{
    return ::asGetLibraryOptions();
}

/* ------------------------------------------------------------------ */
/* Module */
/* ------------------------------------------------------------------ */

AS_API int asModule_AddRef(asIScriptModule *m)
{
    return m ? m->AddRef() : 0;
}

AS_API int asModule_Release(asIScriptModule *m)
{
    return m ? m->Release() : 0;
}

AS_API int asModule_AddScriptSection(asIScriptModule *m, const char *name, const char *code, size_t codeLength, int lineOffset)
{
    if (!m) return asINVALID_ARG;
    return m->AddScriptSection(name, code, codeLength, lineOffset);
}

AS_API int asModule_Build(asIScriptModule *m)
{
    return m ? m->Build() : asINVALID_ARG;
}

AS_API asIScriptFunction *asModule_GetFunctionByDecl(asIScriptModule *m, const char *decl)
{
    return m ? m->GetFunctionByDecl(decl) : 0;
}

AS_API asIScriptFunction *asModule_GetFunctionByName(asIScriptModule *m, const char *name)
{
    return m ? m->GetFunctionByName(name) : 0;
}

AS_API int asModule_GetFunctionCount(asIScriptModule *m)
{
    return m ? m->GetFunctionCount() : 0;
}

/* ------------------------------------------------------------------ */
/* Context */
/* ------------------------------------------------------------------ */

AS_API int asContext_AddRef(asIScriptContext *c)
{
    return c ? c->AddRef() : 0;
}

AS_API int asContext_Release(asIScriptContext *c)
{
    return c ? c->Release() : 0;
}

AS_API int asContext_Prepare(asIScriptContext *c, asIScriptFunction *func)
{
    if (!c) return asINVALID_ARG;
    return c->Prepare(func);
}

AS_API int asContext_Execute(asIScriptContext *c)
{
    return c ? c->Execute() : asINVALID_ARG;
}

AS_API int asContext_Abort(asIScriptContext *c)
{
    return c ? c->Abort() : asINVALID_ARG;
}

AS_API int asContext_Suspend(asIScriptContext *c)
{
    return c ? c->Suspend() : asINVALID_ARG;
}

AS_API void *asContext_GetAddressOfArg(asIScriptContext *c, asUINT arg)
{
    return c ? c->GetAddressOfArg(arg) : 0;
}

AS_API void *asContext_GetAddressOfReturnValue(asIScriptContext *c)
{
    return c ? c->GetAddressOfReturnValue() : 0;
}

AS_API int asContext_SetArgObject(asIScriptContext *c, asUINT arg, void *obj)
{
    if (!c) return asINVALID_ARG;
    return c->SetArgObject(arg, obj);
}

AS_API int asContext_SetArgByte(asIScriptContext *c, asUINT arg, unsigned char value)
{
    if (!c) return asINVALID_ARG;
    return c->SetArgByte(arg, value);
}

AS_API int asContext_SetArgWord(asIScriptContext *c, asUINT arg, unsigned short value)
{
    if (!c) return asINVALID_ARG;
    return c->SetArgWord(arg, value);
}

AS_API int asContext_SetArgDWord(asIScriptContext *c, asUINT arg, asDWORD value)
{
    if (!c) return asINVALID_ARG;
    return c->SetArgDWord(arg, value);
}

AS_API int asContext_SetArgQWord(asIScriptContext *c, asUINT arg, asQWORD value)
{
    if (!c) return asINVALID_ARG;
    return c->SetArgQWord(arg, value);
}

AS_API int asContext_SetArgFloat(asIScriptContext *c, asUINT arg, float value)
{
    if (!c) return asINVALID_ARG;
    return c->SetArgFloat(arg, value);
}

AS_API int asContext_SetArgDouble(asIScriptContext *c, asUINT arg, double value)
{
    if (!c) return asINVALID_ARG;
    return c->SetArgDouble(arg, value);
}

AS_API int asContext_SetArgAddress(asIScriptContext *c, asUINT arg, void *addr)
{
    if (!c) return asINVALID_ARG;
    return c->SetArgAddress(arg, addr);
}

AS_API asDWORD asContext_GetReturnDWord(asIScriptContext *c)
{
    return c ? c->GetReturnDWord() : 0;
}

AS_API asQWORD asContext_GetReturnQWord(asIScriptContext *c)
{
    return c ? c->GetReturnQWord() : 0;
}

AS_API float asContext_GetReturnFloat(asIScriptContext *c)
{
    return c ? c->GetReturnFloat() : 0.0f;
}

AS_API double asContext_GetReturnDouble(asIScriptContext *c)
{
    return c ? c->GetReturnDouble() : 0.0;
}

AS_API void *asContext_GetReturnObject(asIScriptContext *c)
{
    return c ? c->GetReturnObject() : 0;
}

AS_API void *asContext_GetReturnAddress(asIScriptContext *c)
{
    return c ? c->GetReturnAddress() : 0;
}

AS_API int asContext_GetState(asIScriptContext *c)
{
    return c ? c->GetState() : 0;
}

AS_API int asContext_GetExceptionLineNumber(asIScriptContext *c, int *column, const char **sectionName)
{
    return c ? c->GetExceptionLineNumber(column, sectionName) : 0;
}

AS_API const char *asContext_GetExceptionString(asIScriptContext *c)
{
    return c ? c->GetExceptionString() : 0;
}

} /* extern "C" */
