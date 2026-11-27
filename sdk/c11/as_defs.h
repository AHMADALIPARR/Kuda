// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* as_defs.h - constants for the C11 AngelScript-subset engine.
 *
 * The dropped angelscript_c.h declares the C API functions but does not
 * define the version, calling-convention, module-flag, execution-state,
 * or return-code constants that callers use (those live in the upstream
 * angelscript.h, which this engine deliberately does not use). This header
 * supplies them with the exact values of AngelScript 2.39.0 so existing
 * callers (tests/test_c11_bridge.c) compile without edits. The build
 * force-includes this header (-include as_defs.h) when compiling callers;
 * engine sources include it directly.
 *
 * This is a clean-room subset: only the constants the subset implements
 * are defined. No SDK code is used.
 */

#ifndef AS_C11_DEFS_H
#define AS_C11_DEFS_H

#define ANGELSCRIPT_VERSION 23900
#define ANGELSCRIPT_VERSION_STRING "2.39.0"

/* Calling conventions (asECallConv) */
#define asCALL_CDECL             0
#define asCALL_STDCALL           1
#define asCALL_THISCALL          2
#define asCALL_CDECL_OBJLAST     3
#define asCALL_CDECL_OBJFIRST    4
#define asCALL_GENERIC           5
#define asCALL_THISCALL_OBJLAST  6
#define asCALL_THISCALL_OBJFIRST 7

/* Object behaviours (asEBehaviours) - recognised, not implemented */
#define asBEHAVE_CONSTRUCT       0
#define asBEHAVE_LIST_CONSTRUCT  1
#define asBEHAVE_DESTRUCT        2
#define asBEHAVE_FACTORY         3
#define asBEHAVE_LIST_FACTORY    4
#define asBEHAVE_ADDREF          5
#define asBEHAVE_RELEASE         6
#define asBEHAVE_GET_WEAKREF_FLAG 7
#define asBEHAVE_TEMPLATE_CALLBACK 8

/* Function types (asEFuncType) */
#define asFUNC_DUMMY     -1
#define asFUNC_SYSTEM     0
#define asFUNC_SCRIPT     1
#define asFUNC_INTERFACE  2
#define asFUNC_VIRTUAL    3
#define asFUNC_FUNCDEF    4
#define asFUNC_IMPORTED   5
#define asFUNC_DELEGATE   6

/* Context / execution states (asEContextState) */
#define asEXECUTION_FINISHED      0
#define asEXECUTION_SUSPENDED     1
#define asEXECUTION_ABORTED       2
#define asEXECUTION_EXCEPTION     3
#define asEXECUTION_PREPARED      4
#define asEXECUTION_UNINITIALIZED 5
#define asEXECUTION_ACTIVE        6
#define asEXECUTION_ERROR         7

/* Module creation flags (asEGMFlags) */
#define asGM_ONLY_IF_EXISTS       0
#define asGM_CREATE_IF_NOT_EXISTS 1
#define asGM_ALWAYS_CREATE        2

/* Return codes (asERetCodes) */
#define asSUCCESS              0
#define asERROR               -1
#define asCONTEXT_ACTIVE      -2
#define asCONTEXT_NOT_FINISHED -3
#define asCONTEXT_NOT_PREPARED -4
#define asINVALID_ARG         -5
#define asNO_FUNCTION         -6
#define asNOT_SUPPORTED       -7
#define asINVALID_TYPE        -8
#define asALREADY_REGISTERED  -9
#define asINVALID_DECLARATION -10
#define asINVALID_OBJECT      -11
#define asINVALID_MODULE      -12
#define asINVALID_CONFIGURATION -13
#define asINVALID_TYPEDEF     -14
#define asINVALID_QUALIFIER   -15
#define asINVALID_OPERATION   -16
#define asINVALID_ENUM        -17
#define asNOT_INITIALIZED     -18
#define asNOTHING_TO_DO       -19
#define asNOT_IMPLEMENTED     -20
#define asMODULE_IS_IN_USE    -21
#define asINVALID_VERSION     -22
#define asNO_FUNCTION_FOUND   -23

/* Object type flags (asEObjectFlags) - recognised, not implemented */
#define asOBJ_REF            (1 << 0)
#define asOBJ_VALUE          (1 << 1)
#define asOBJ_GC             (1 << 2)
#define asOBJ_POD            (1 << 3)
#define asOBJ_NOHANDLE       (1 << 4)
#define asOBJ_SCOPED         (1 << 5)
#define asOBJ_TEMPLATE       (1 << 6)
#define asOBJ_ASHANDLE       (1 << 7)
#define asOBJ_APP_CLASS      (1 << 8)
#define asOBJ_APP_CLASS_C    (1 << 9)
#define asOBJ_APP_CLASS_D    (1 << 10)
#define asOBJ_APP_CLASS_CD   (1 << 11)
#define asOBJ_APP_CLASS_MORE (1 << 12)

/* Message types (asEMsgType) */
#define asMSGTYPE_ERROR       0
#define asMSGTYPE_WARNING     1
#define asMSGTYPE_INFORMATION 2

/* Compiler diagnostic record passed to the message callback. */
typedef struct asSMessageInfo {
    const char *section;
    int row;
    int col;
    int type; /* asMSGTYPE_* */
    const char *message;
} asSMessageInfo;

#endif /* AS_C11_DEFS_H */
