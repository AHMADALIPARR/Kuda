// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* as_c11.h - internal shared header for the C11 AngelScript-subset engine.
 * Value model, AST, lexer, parser, interpreter and engine internals.
 * The public C API surface is angelscript_c.h (+ as_defs.h constants).
 * Pure C11, no C++, no third-party code.
 */

#ifndef AS_C11_H
#define AS_C11_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "as_defs.h"
#include "angelscript_c.h"
#include "bridge.h" /* reverse-flow-machine execution spine (ffi/) */

/* ------------------------------------------------------------------ */
/* Types and values                                                    */
/* ------------------------------------------------------------------ */

/* Subset type kinds. Narrow integer keywords (int8/int16/uint8/uint16/
 * uint32) are accepted as aliases of int/uint at parse time. */
typedef enum {
    AS_T_VOID = 0,
    AS_T_BOOL,
    AS_T_INT,    /* int32 */
    AS_T_INT64,
    AS_T_UINT,   /* uint32 */
    AS_T_UINT64,
    AS_T_FLOAT,
    AS_T_DOUBLE
} as_type;

typedef struct {
    as_type kind;
    union {
        int32_t i32;
        int64_t i64;
        uint32_t u32;
        uint64_t u64;
        float f32;
        double f64;
        int b; /* bool as 0/1 */
    } v;
} as_value;

as_value as_value_void(void);
as_value as_value_bool(int b);
as_value as_value_int(int32_t x);
as_value as_value_int64(int64_t x);
as_value as_value_uint(uint32_t x);
as_value as_value_uint64(uint64_t x);
as_value as_value_float(float x);
as_value as_value_double(double x);

/* Cast a value to a target type (numeric conversions, C rules). */
int as_value_cast(const as_value *in, as_type to, as_value *out);
/* Truthiness: 0/false/0.0 is false, anything else true. */
int as_value_truthy(const as_value *v);
/* Common arithmetic type for a binary operation (C promotions). */
as_type as_common_type(as_type a, as_type b);
int as_type_is_integer(as_type t);
int as_type_is_float(as_type t);
int as_type_is_numeric(as_type t);
const char *as_type_name(as_type t);
int as_type_from_keyword(const char *kw); /* -1 when not a type keyword */

/* ------------------------------------------------------------------ */
/* AST                                                                 */
/* ------------------------------------------------------------------ */

typedef enum {
    AST_PROGRAM,
    AST_FUNC,       /* kids[0]=params node, kids[1]=body BLOCK */
    AST_PARAMLIST,  /* kids = PARAM nodes */
    AST_PARAM,      /* name, type */
    AST_BLOCK,      /* kids = statements */
    AST_VARDECL,    /* name, type, kids[0]=init expr or absent */
    AST_IF,         /* kids[0]=cond, kids[1]=then, kids[2]=else or absent */
    AST_WHILE,      /* kids[0]=cond, kids[1]=body */
    AST_FOR,        /* kids[0]=init stmt/NULL, kids[1]=cond/NULL,
                       kids[2]=incr/NULL, kids[3]=body */
    AST_RETURN,     /* kids[0]=expr or absent */
    AST_BREAK,
    AST_CONTINUE,
    AST_EXPRSTMT,   /* kids[0]=expr */
    AST_ASSIGN,     /* op, kids[0]=target IDENT, kids[1]=value */
    AST_BINOP,      /* op, kids[0]=left, kids[1]=right */
    AST_UNOP,       /* op, kids[0]=operand, aux=1 when postfix */
    AST_CAST,       /* type, kids[0]=operand */
    AST_CALL,       /* name, kids = args */
    AST_IDENT,      /* name */
    AST_LIT_INT, AST_LIT_INT64, AST_LIT_UINT, AST_LIT_UINT64,
    AST_LIT_FLOAT, AST_LIT_DOUBLE, AST_LIT_BOOL
} as_ast_kind;

typedef struct as_ast {
    as_ast_kind kind;
    int line;
    int op;   /* operator token for BINOP/UNOP/ASSIGN */
    int type; /* declared type (as_type) for VARDECL/FUNC/PARAM */
    int aux;  /* UNOP: 1 when postfix ++/-- */
    char *name; /* identifier / function name */
    struct as_ast **kids;
    int nkids, capkids;
    int64_t ival;
    uint64_t uval;
    double fval;
    int bval;
} as_ast;

as_ast *as_ast_new(as_ast_kind kind, int line);
void as_ast_addkid(as_ast *n, as_ast *kid);
void as_ast_free(as_ast *n);
char *as_xstrdup(const char *s);
char *as_xstrndup(const char *s, size_t n);
void *as_xmalloc(size_t n);
void *as_xrealloc(void *p, size_t n);

/* ------------------------------------------------------------------ */
/* Lexer                                                               */
/* ------------------------------------------------------------------ */

typedef enum {
    TOK_EOF = 0, TOK_IDENT, TOK_INT, TOK_FLOAT, TOK_STRING, TOK_CHAR,
    TOK_VOID, TOK_BOOL, TOK_INT_KW, TOK_INT8, TOK_INT16, TOK_INT64,
    TOK_UINT, TOK_UINT8, TOK_UINT16, TOK_UINT32, TOK_UINT64,
    TOK_FLOAT_KW, TOK_DOUBLE, TOK_TRUE, TOK_FALSE,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_RETURN, TOK_BREAK,
    TOK_CONTINUE, TOK_CONST,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN, TOK_STAR_ASSIGN,
    TOK_SLASH_ASSIGN, TOK_PERCENT_ASSIGN,
    TOK_EQ, TOK_NE, TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_BITAND, TOK_BITOR, TOK_BITXOR, TOK_BITNOT,
    TOK_SHL, TOK_SHR, TOK_SHL_ASSIGN, TOK_SHR_ASSIGN,
    TOK_BITAND_ASSIGN, TOK_BITOR_ASSIGN, TOK_BITXOR_ASSIGN,
    TOK_INC, TOK_DEC,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET,
    TOK_SEMI, TOK_COMMA, TOK_COLON, TOK_QUESTION, TOK_DOT
} as_tok_kind;

typedef struct {
    as_tok_kind kind;
    int line;
    char *text;   /* identifier / literal spelling (owned) */
    int64_t ival; /* integer literal value */
    double fval;  /* float literal value */
    int is_float; /* literal has '.'/'e' or an 'f' suffix */
    int is_unsigned;
    int is_long;  /* 'l'/'ll' suffix */
    int is_float_suffix; /* 'f' suffix */
} as_token;

typedef struct {
    const char *src;   /* source text (not owned) */
    const char *section;
    size_t pos;
    int line;
    as_token cur;      /* current token */
    as_token peek;     /* one token of lookahead */
    int failed;        /* a lex error was recorded */
    char errmsg[256];
} as_lexer;

void as_lexer_init(as_lexer *lx, const char *src, const char *section);
void as_lexer_next(as_lexer *lx); /* advance cur <- peek, lex a new peek */
void as_lexer_free(as_lexer *lx);
const char *as_token_name(as_tok_kind k);

/* ------------------------------------------------------------------ */
/* Parser                                                              */
/* ------------------------------------------------------------------ */

typedef struct as_parser {
    as_lexer lx;
    int failed;
    char errmsg[256];
    int errline;
} as_parser;

/* Parse a whole program (list of function definitions). Returns NULL and
 * sets parser->failed + errmsg on error. */
as_ast *as_parse_program(as_parser *p, const char *src, const char *section);
void as_parser_init(as_parser *p);
void as_parser_free(as_parser *p);

/* Parse a declaration string ("void print()", "float f(int, uint)") into
 * return type, name, and parameter types. Used by RegisterGlobalFunction
 * and GetFunctionByDecl. Returns 0 on success. */
typedef struct {
    as_type ret;
    char *name; /* owned */
    as_type *params; /* owned array */
    int nparams;
    int failed;
    char errmsg[256];
} as_decl;
int as_parse_decl(const char *decl, as_decl *out);
void as_decl_free(as_decl *d);

/* ------------------------------------------------------------------ */
/* Interpreter                                                         */
/* ------------------------------------------------------------------ */

#define AS_MAX_ARGS 16
#define AS_DEFAULT_MAX_STEPS 1000000ul
#define AS_DEFAULT_MAX_CALL_DEPTH 64

struct asIScriptEngine;
struct asIScriptModule;
struct asIScriptContext;
struct asIScriptFunction;

/* Execution state shared by the interpreter and the context.
 * The step budget lives in the reverse-flow-machine (ctx->rf.fuel);
 * each interpreter step burns one unit of fuel through rf_step's
 * accounting. The call-depth limit stays in the interpreter. */
typedef struct as_exec {
    struct asIScriptContext *ctx;
    struct asIScriptEngine *engine;
    struct asIScriptModule *module;
    int call_depth;
    int aborted; /* set by asContext_Abort */
} as_exec;

/* Control-flow signal returned by statement execution. */
#define FLOW_NORMAL 0
#define FLOW_BREAK 1
#define FLOW_CONTINUE 2
#define FLOW_RETURN 3

/* Validate a parsed program: every variable reference must be declared in
 * an enclosing scope and every call target must be a script function of
 * the module or a registered global function. Returns 0 on success;
 * on failure fills errmsg and returns -1. */
int as_validate_program(struct asIScriptEngine *eng,
                        struct asIScriptModule *mod,
                        as_ast *prog, char *errmsg, size_t errcap,
                        int *errline);

/* Execute a script function. Returns 0 on normal completion (retVal set),
 * -1 on exception (ctx exception fields set). */
int as_exec_function(as_exec *ex, struct asIScriptFunction *fn,
                     as_value *args, int nargs, as_value *ret);

/* ------------------------------------------------------------------ */
/* Engine internals (opaque structs defined in as_engine.c)           */
/* ------------------------------------------------------------------ */

typedef struct as_reg_func {
    char *decl; /* original declaration text (owned) */
    char *name; /* owned */
    as_type ret;
    as_type *params; /* owned */
    int nparams;
    asFUNCTION_t func;
    asDWORD callConv;
    int id;
} as_reg_func;

struct asIScriptFunction {
    int refCount;
    struct asIScriptModule *module; /* NULL for registered functions */
    char *name; /* owned */
    char *decl; /* canonical declaration text (owned) */
    as_type ret;
    as_type *params; /* owned */
    int nparams;
    as_ast *ast; /* function-definition node, NULL for registered */
    int is_script;
};

struct asIScriptModule {
    int refCount;
    struct asIScriptEngine *engine;
    char *name; /* owned */
    struct as_section {
        char *name; /* owned */
        char *code; /* owned, NUL-terminated */
        size_t len;
        int lineOffset;
    } *sections;
    int nsections, capsections;
    struct asIScriptFunction **funcs;
    int nfuncs, capfuncs;
    as_ast *program; /* parsed program, NULL until built */
    int built;
};

struct asIScriptContext {
    int refCount;
    struct asIScriptEngine *engine;
    struct asIScriptFunction *func; /* prepared function */
    int state; /* asEContextState */
    as_value args[AS_MAX_ARGS];
    int argset[AS_MAX_ARGS];
    as_value retval;
    int has_retval;
    char exception[256];
    int exception_line;
    int exception_col;
    char exception_section[64];
    unsigned long max_steps;
    int max_call_depth;
    int abort_flag;
    /* Reverse-flow-machine execution spine. rf.fuel is the step budget
     * (initialised to max_steps + lifecycle headroom in Prepare);
     * rf.phase tracks the execution lifecycle (PENDING -> VALIDATED ->
     * COMMITTED/REJECTED); rf.trace records the transitions for
     * deterministic replay (rf_replay_verify). */
    rf_machine_t rf;
};

struct asIScriptEngine {
    int refCount;
    asMESSAGEFUNC_t msg_cb;
    void *msg_cb_obj;
    asDWORD msg_callconv;
    as_reg_func *regfuncs;
    int nregfuncs, capregfuncs;
    struct asIScriptModule **modules;
    int nmodules, capmodules;
};

/* Engine helpers used by the interpreter. */
void as_engine_message(struct asIScriptEngine *e, const char *section,
                       int row, int col, int type, const char *msg);
struct asIScriptFunction *as_engine_find_script_func(
    struct asIScriptModule *m, const char *name, int nargs);
as_reg_func *as_engine_find_reg_func(struct asIScriptEngine *e,
                                     const char *name, int nargs);
int as_engine_call_registered(struct asIScriptContext *ctx,
                              as_reg_func *rf, as_value *args, int nargs,
                              as_value *ret);

#endif /* AS_C11_H */
