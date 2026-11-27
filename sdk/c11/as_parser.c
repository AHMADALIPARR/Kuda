// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* as_parser.c - recursive-descent parser + AST for the C11 subset.
 * Functions, variable declarations, expressions, control flow.
 * Also parses declaration strings ("void print()") for the C API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "as_c11.h"

/* ---------------- AST ---------------- */

as_ast *as_ast_new(as_ast_kind kind, int line)
{
    as_ast *n = as_xmalloc(sizeof *n);
    memset(n, 0, sizeof *n);
    n->kind = kind;
    n->line = line;
    n->type = AS_T_VOID;
    return n;
}

void as_ast_addkid(as_ast *n, as_ast *kid)
{
    if (n->nkids >= n->capkids) {
        n->capkids = n->capkids ? n->capkids * 2 : 4;
        n->kids = as_xrealloc(n->kids, (size_t)n->capkids * sizeof *n->kids);
    }
    n->kids[n->nkids++] = kid;
}

void as_ast_free(as_ast *n)
{
    int i;
    if (!n) return;
    for (i = 0; i < n->nkids; i++) as_ast_free(n->kids[i]);
    free(n->kids);
    free(n->name);
    free(n);
}

/* ---------------- parser helpers ---------------- */

static int p_cur(as_parser *p) { return (int)p->lx.cur.kind; }
static int p_peek(as_parser *p) { return (int)p->lx.peek.kind; }
static int p_line(as_parser *p) { return p->lx.cur.line; }

static void p_advance(as_parser *p) { as_lexer_next(&p->lx); }

static void p_fail(as_parser *p, const char *msg)
{
    if (p->failed) return;
    p->failed = 1;
    p->errline = p->lx.cur.line;
    snprintf(p->errmsg, sizeof p->errmsg, "%s", msg);
}

static int p_expect(as_parser *p, as_tok_kind k, const char *what)
{
    if (p_cur(p) != (int)k) {
        char buf[160];
        snprintf(buf, sizeof buf, "expected %s, got %s",
                 what, as_token_name(p->lx.cur.kind));
        p_fail(p, buf);
        return 0;
    }
    p_advance(p);
    return 1;
}

static int p_is_type_kw(as_tok_kind k)
{
    return k == TOK_VOID || k == TOK_BOOL || k == TOK_INT_KW ||
           k == TOK_INT8 || k == TOK_INT16 || k == TOK_INT64 ||
           k == TOK_UINT || k == TOK_UINT8 || k == TOK_UINT16 ||
           k == TOK_UINT32 || k == TOK_UINT64 ||
           k == TOK_FLOAT_KW || k == TOK_DOUBLE;
}

/* Parse [const] type. Returns the as_type, or -1 (not a type). */
static int p_parse_type(as_parser *p)
{
    int t = -1;
    if (p_cur(p) == TOK_CONST) p_advance(p);
    if (!p_is_type_kw(p_cur(p))) return -1;
    t = as_type_from_keyword(p->lx.cur.text);
    p_advance(p);
    return t;
}

/* Forward declarations */
static as_ast *parse_expr(as_parser *p);
static as_ast *parse_statement(as_parser *p);
static as_ast *parse_block(as_parser *p);

/* ---------------- expressions ---------------- */

static as_ast *parse_primary(as_parser *p)
{
    as_ast *n;
    int line = p_line(p);
    switch (p_cur(p)) {
    case TOK_INT: {
        int64_t v = p->lx.cur.ival;
        int is_u = p->lx.cur.is_unsigned;
        int is_l = p->lx.cur.is_long;
        p_advance(p);
        if (is_u) {
            if (is_l || v > 0xFFFFFFFFLL || v < 0) {
                n = as_ast_new(AST_LIT_UINT64, line);
                n->uval = (uint64_t)v;
            } else {
                n = as_ast_new(AST_LIT_UINT, line);
                n->uval = (uint64_t)v;
            }
        } else if (is_l || v > 2147483647LL || v < -2147483648LL) {
            n = as_ast_new(AST_LIT_INT64, line);
            n->ival = v;
        } else {
            n = as_ast_new(AST_LIT_INT, line);
            n->ival = v;
        }
        return n;
    }
    case TOK_FLOAT: {
        double v = p->lx.cur.fval;
        int is_f = p->lx.cur.is_float_suffix;
        p_advance(p);
        if (is_f) {
            n = as_ast_new(AST_LIT_FLOAT, line);
            n->fval = v;
        } else {
            n = as_ast_new(AST_LIT_DOUBLE, line);
            n->fval = v;
        }
        return n;
    }
    case TOK_TRUE:
        p_advance(p);
        n = as_ast_new(AST_LIT_BOOL, line);
        n->bval = 1;
        return n;
    case TOK_FALSE:
        p_advance(p);
        n = as_ast_new(AST_LIT_BOOL, line);
        n->bval = 0;
        return n;
    case TOK_CHAR: {
        int64_t v = p->lx.cur.ival;
        p_advance(p);
        n = as_ast_new(AST_LIT_INT, line);
        n->ival = v;
        return n;
    }
    case TOK_STRING:
        p_fail(p, "string literals are not supported in this subset");
        return NULL;
    case TOK_IDENT: {
        char *name = as_xstrdup(p->lx.cur.text);
        p_advance(p);
        if (p_cur(p) == TOK_LPAREN) {
            /* call */
            p_advance(p);
            n = as_ast_new(AST_CALL, line);
            n->name = name;
            if (p_cur(p) != TOK_RPAREN) {
                for (;;) {
                    as_ast *a = parse_expr(p);
                    if (!a) { as_ast_free(n); return NULL; }
                    as_ast_addkid(n, a);
                    if (p_cur(p) == TOK_COMMA) { p_advance(p); continue; }
                    break;
                }
            }
            if (!p_expect(p, TOK_RPAREN, "')'")) { as_ast_free(n); return NULL; }
            return n;
        }
        n = as_ast_new(AST_IDENT, line);
        n->name = name;
        return n;
    }
    case TOK_LPAREN: {
        as_ast *e;
        p_advance(p);
        e = parse_expr(p);
        if (!e) return NULL;
        if (!p_expect(p, TOK_RPAREN, "')'")) { as_ast_free(e); return NULL; }
        return e;
    }
    default:
        p_fail(p, "expected an expression");
        return NULL;
    }
}

static as_ast *parse_postfix(as_parser *p)
{
    as_ast *e = parse_primary(p);
    if (!e) return NULL;
    for (;;) {
        int line = p_line(p);
        if (p_cur(p) == TOK_INC || p_cur(p) == TOK_DEC) {
            as_ast *u = as_ast_new(AST_UNOP, line);
            u->op = p_cur(p);
            u->aux = 1; /* postfix */
            as_ast_addkid(u, e);
            p_advance(p);
            e = u;
            continue;
        }
        if (p_cur(p) == TOK_DOT) {
            p_fail(p, "member access ('.') is not supported in this subset");
            as_ast_free(e);
            return NULL;
        }
        if (p_cur(p) == TOK_LBRACKET) {
            p_fail(p, "array indexing ('[]') is not supported in this subset");
            as_ast_free(e);
            return NULL;
        }
        break;
    }
    return e;
}

static as_ast *parse_unary(as_parser *p)
{
    int line = p_line(p);
    if (p_cur(p) == TOK_MINUS || p_cur(p) == TOK_NOT ||
        p_cur(p) == TOK_BITNOT || p_cur(p) == TOK_INC ||
        p_cur(p) == TOK_DEC) {
        int op = p_cur(p);
        as_ast *u, *operand;
        p_advance(p);
        operand = parse_unary(p);
        if (!operand) return NULL;
        u = as_ast_new(AST_UNOP, line);
        u->op = op;
        u->aux = 0; /* prefix */
        as_ast_addkid(u, operand);
        return u;
    }
    if (p_cur(p) == TOK_LPAREN &&
        (p_is_type_kw(p_peek(p)) || p_peek(p) == TOK_CONST)) {
        /* C-style cast: (type) unary-expr. A parenthesised expression
         * cannot start with a type keyword or 'const', so this is
         * unambiguous. */
        as_ast *c, *operand;
        int t;
        p_advance(p); /* consume '(' */
        t = p_parse_type(p);
        if (t < 0) { p_fail(p, "expected a type in cast"); return NULL; }
        if (!p_expect(p, TOK_RPAREN, "')'")) return NULL;
        operand = parse_unary(p);
        if (!operand) return NULL;
        c = as_ast_new(AST_CAST, line);
        c->type = t;
        as_ast_addkid(c, operand);
        return c;
    }
    return parse_postfix(p);
}

typedef as_ast *(*binop_fn)(as_parser *p);

static as_ast *parse_binop_level(as_parser *p, binop_fn sub,
                                 const int *ops, int nops)
{
    as_ast *left = sub(p);
    int i;
    if (!left) return NULL;
    for (;;) {
        int op = -1;
        for (i = 0; i < nops; i++) {
            if (p_cur(p) == ops[i]) { op = ops[i]; break; }
        }
        if (op < 0) break;
        {
            int line = p_line(p);
            as_ast *right, *n;
            p_advance(p);
            right = sub(p);
            if (!right) { as_ast_free(left); return NULL; }
            n = as_ast_new(AST_BINOP, line);
            n->op = op;
            as_ast_addkid(n, left);
            as_ast_addkid(n, right);
            left = n;
        }
    }
    return left;
}

static as_ast *parse_multiplicative(as_parser *p)
{
    static const int ops[] = { TOK_STAR, TOK_SLASH, TOK_PERCENT };
    return parse_binop_level(p, parse_unary, ops, 3);
}

static as_ast *parse_additive(as_parser *p)
{
    static const int ops[] = { TOK_PLUS, TOK_MINUS };
    return parse_binop_level(p, parse_multiplicative, ops, 2);
}

static as_ast *parse_shift(as_parser *p)
{
    static const int ops[] = { TOK_SHL, TOK_SHR };
    return parse_binop_level(p, parse_additive, ops, 2);
}

static as_ast *parse_relational(as_parser *p)
{
    static const int ops[] = { TOK_LT, TOK_LE, TOK_GT, TOK_GE };
    return parse_binop_level(p, parse_shift, ops, 4);
}

static as_ast *parse_equality(as_parser *p)
{
    static const int ops[] = { TOK_EQ, TOK_NE };
    return parse_binop_level(p, parse_relational, ops, 2);
}

static as_ast *parse_bitand(as_parser *p)
{
    static const int ops[] = { TOK_BITAND };
    return parse_binop_level(p, parse_equality, ops, 1);
}

static as_ast *parse_bitxor(as_parser *p)
{
    static const int ops[] = { TOK_BITXOR };
    return parse_binop_level(p, parse_bitand, ops, 1);
}

static as_ast *parse_bitor(as_parser *p)
{
    static const int ops[] = { TOK_BITOR };
    return parse_binop_level(p, parse_bitxor, ops, 1);
}

static as_ast *parse_and(as_parser *p)
{
    static const int ops[] = { TOK_AND };
    return parse_binop_level(p, parse_bitor, ops, 1);
}

static as_ast *parse_or(as_parser *p)
{
    static const int ops[] = { TOK_OR };
    return parse_binop_level(p, parse_and, ops, 1);
}

static int p_is_assign_op(int k)
{
    return k == TOK_ASSIGN || k == TOK_PLUS_ASSIGN || k == TOK_MINUS_ASSIGN ||
           k == TOK_STAR_ASSIGN || k == TOK_SLASH_ASSIGN ||
           k == TOK_PERCENT_ASSIGN || k == TOK_SHL_ASSIGN ||
           k == TOK_SHR_ASSIGN || k == TOK_BITAND_ASSIGN ||
           k == TOK_BITOR_ASSIGN || k == TOK_BITXOR_ASSIGN;
}

static as_ast *parse_expr(as_parser *p)
{
    as_ast *left = parse_or(p);
    int line;
    if (!left) return NULL;
    if (!p_is_assign_op(p_cur(p))) return left;
    if (left->kind != AST_IDENT) {
        p_fail(p, "assignment target must be a variable");
        as_ast_free(left);
        return NULL;
    }
    line = p_line(p);
    {
        int op = p_cur(p);
        as_ast *value, *n;
        p_advance(p);
        value = parse_expr(p); /* right-associative */
        if (!value) { as_ast_free(left); return NULL; }
        n = as_ast_new(AST_ASSIGN, line);
        n->op = op;
        as_ast_addkid(n, left);
        as_ast_addkid(n, value);
        return n;
    }
}

/* ---------------- statements ---------------- */

/* Parse a variable declaration (type already consumed by caller in the
 * for-init case is handled by the caller). This parses:
 *   type name [= expr] (, name [= expr])* ;
 * When semi is 0, the trailing ';' is NOT consumed (for-init use). */
static as_ast *parse_vardecl_rest(as_parser *p, int type, int line, int semi)
{
    as_ast *block = as_ast_new(AST_BLOCK, line);
    block->aux = 1; /* marker: this BLOCK is a declaration list */
    for (;;) {
        as_ast *d;
        char *name;
        if (p_cur(p) != TOK_IDENT) {
            p_fail(p, "expected a variable name in declaration");
            as_ast_free(block);
            return NULL;
        }
        name = as_xstrdup(p->lx.cur.text);
        p_advance(p);
        d = as_ast_new(AST_VARDECL, line);
        d->type = type;
        d->name = name;
        if (p_cur(p) == TOK_ASSIGN) {
            as_ast *init;
            p_advance(p);
            init = parse_expr(p);
            if (!init) { as_ast_free(d); as_ast_free(block); return NULL; }
            as_ast_addkid(d, init);
        }
        as_ast_addkid(block, d);
        if (p_cur(p) == TOK_COMMA) { p_advance(p); continue; }
        break;
    }
    if (semi) {
        if (!p_expect(p, TOK_SEMI, "';'")) { as_ast_free(block); return NULL; }
    }
    /* A declaration list is returned as a BLOCK of VARDECL nodes; the
     * caller unwraps it when it is a single statement. */
    return block;
}

static as_ast *parse_block(as_parser *p)
{
    int line = p_line(p);
    as_ast *b = as_ast_new(AST_BLOCK, line);
    if (!p_expect(p, TOK_LBRACE, "'{'")) { as_ast_free(b); return NULL; }
    while (p_cur(p) != TOK_RBRACE && p_cur(p) != TOK_EOF && !p->failed) {
        as_ast *s = parse_statement(p);
        if (!s) { as_ast_free(b); return NULL; }
        /* A declaration list comes back as a BLOCK with aux==1; splice
         * its VARDECL children into this block. */
        if (s->kind == AST_BLOCK && s->aux == 1) {
            int i;
            for (i = 0; i < s->nkids; i++) as_ast_addkid(b, s->kids[i]);
            /* steal children so free() does not free them */
            s->nkids = 0;
            as_ast_free(s);
        } else {
            as_ast_addkid(b, s);
        }
    }
    if (!p_expect(p, TOK_RBRACE, "'}'")) { as_ast_free(b); return NULL; }
    return b;
}

static as_ast *parse_statement(as_parser *p)
{
    int line = p_line(p);
    as_ast *n;
    if (p_cur(p) == TOK_LBRACE) return parse_block(p);
    if (p_cur(p) == TOK_IF) {
        as_ast *cond, *then, *els = NULL;
        p_advance(p);
        if (!p_expect(p, TOK_LPAREN, "'('")) return NULL;
        cond = parse_expr(p);
        if (!cond) return NULL;
        if (!p_expect(p, TOK_RPAREN, "')'")) { as_ast_free(cond); return NULL; }
        then = parse_statement(p);
        if (!then) { as_ast_free(cond); return NULL; }
        if (p_cur(p) == TOK_ELSE) {
            p_advance(p);
            els = parse_statement(p);
            if (!els) { as_ast_free(cond); as_ast_free(then); return NULL; }
        }
        n = as_ast_new(AST_IF, line);
        as_ast_addkid(n, cond);
        as_ast_addkid(n, then);
        if (els) as_ast_addkid(n, els);
        return n;
    }
    if (p_cur(p) == TOK_WHILE) {
        as_ast *cond, *body;
        p_advance(p);
        if (!p_expect(p, TOK_LPAREN, "'('")) return NULL;
        cond = parse_expr(p);
        if (!cond) return NULL;
        if (!p_expect(p, TOK_RPAREN, "')'")) { as_ast_free(cond); return NULL; }
        body = parse_statement(p);
        if (!body) { as_ast_free(cond); return NULL; }
        n = as_ast_new(AST_WHILE, line);
        as_ast_addkid(n, cond);
        as_ast_addkid(n, body);
        return n;
    }
    if (p_cur(p) == TOK_FOR) {
        as_ast *init = NULL, *cond = NULL, *incr = NULL, *body;
        p_advance(p);
        if (!p_expect(p, TOK_LPAREN, "'('")) return NULL;
        if (p_cur(p) != TOK_SEMI) {
            if (p_is_type_kw(p_cur(p)) || p_cur(p) == TOK_CONST) {
                int t = p_parse_type(p);
                if (t < 0) { p_fail(p, "expected a type in for-init"); return NULL; }
                init = parse_vardecl_rest(p, t, line, 0);
                if (!init) return NULL;
            } else {
                init = parse_expr(p);
                if (!init) return NULL;
            }
        }
        if (!p_expect(p, TOK_SEMI, "';'")) {
            as_ast_free(init);
            return NULL;
        }
        if (p_cur(p) != TOK_SEMI) {
            cond = parse_expr(p);
            if (!cond) { as_ast_free(init); return NULL; }
        }
        if (!p_expect(p, TOK_SEMI, "';'")) {
            as_ast_free(init); as_ast_free(cond);
            return NULL;
        }
        if (p_cur(p) != TOK_RPAREN) {
            incr = parse_expr(p);
            if (!incr) { as_ast_free(init); as_ast_free(cond); return NULL; }
        }
        if (!p_expect(p, TOK_RPAREN, "')'")) {
            as_ast_free(init); as_ast_free(cond); as_ast_free(incr);
            return NULL;
        }
        body = parse_statement(p);
        if (!body) {
            as_ast_free(init); as_ast_free(cond); as_ast_free(incr);
            return NULL;
        }
        n = as_ast_new(AST_FOR, line);
        /* kids[0..3] may be NULL; as_ast_addkid cannot store NULL, so use
         * a placeholder empty node and mark it with kind AST_PROGRAM? No:
         * store NULL via direct slot assignment after growing. */
        n->capkids = 4;
        n->kids = as_xmalloc(4 * sizeof *n->kids);
        n->kids[0] = init; n->kids[1] = cond; n->kids[2] = incr; n->kids[3] = body;
        n->nkids = 4;
        return n;
    }
    if (p_cur(p) == TOK_RETURN) {
        p_advance(p);
        n = as_ast_new(AST_RETURN, line);
        if (p_cur(p) != TOK_SEMI) {
            as_ast *e = parse_expr(p);
            if (!e) { as_ast_free(n); return NULL; }
            as_ast_addkid(n, e);
        }
        if (!p_expect(p, TOK_SEMI, "';'")) { as_ast_free(n); return NULL; }
        return n;
    }
    if (p_cur(p) == TOK_BREAK) {
        p_advance(p);
        if (!p_expect(p, TOK_SEMI, "';'")) return NULL;
        return as_ast_new(AST_BREAK, line);
    }
    if (p_cur(p) == TOK_CONTINUE) {
        p_advance(p);
        if (!p_expect(p, TOK_SEMI, "';'")) return NULL;
        return as_ast_new(AST_CONTINUE, line);
    }
    if (p_cur(p) == TOK_SEMI) {
        /* empty statement */
        p_advance(p);
        return as_ast_new(AST_EXPRSTMT, line);
    }
    if (p_is_type_kw(p_cur(p)) || p_cur(p) == TOK_CONST) {
        int t = p_parse_type(p);
        as_ast *d;
        if (t < 0) { p_fail(p, "expected a type"); return NULL; }
        if (t == AS_T_VOID) {
            p_fail(p, "a variable cannot have type void");
            return NULL;
        }
        d = parse_vardecl_rest(p, t, line, 1);
        return d;
    }
    /* expression statement */
    {
        as_ast *e = parse_expr(p);
        if (!e) return NULL;
        if (!p_expect(p, TOK_SEMI, "';'")) { as_ast_free(e); return NULL; }
        n = as_ast_new(AST_EXPRSTMT, line);
        as_ast_addkid(n, e);
        return n;
    }
}

/* ---------------- functions / program ---------------- */

static as_ast *parse_function(as_parser *p)
{
    int line = p_line(p);
    int ret;
    char *name;
    as_ast *fn, *params, *body;
    if (p_cur(p) == TOK_CONST) p_advance(p);
    ret = p_parse_type(p);
    if (ret < 0) {
        p_fail(p, "expected a function return type at top level");
        return NULL;
    }
    if (p_cur(p) != TOK_IDENT) {
        p_fail(p, "expected a function name");
        return NULL;
    }
    name = as_xstrdup(p->lx.cur.text);
    p_advance(p);
    if (!p_expect(p, TOK_LPAREN, "'('")) { free(name); return NULL; }
    params = as_ast_new(AST_PARAMLIST, line);
    if (p_cur(p) != TOK_RPAREN) {
        /* "void" alone means no parameters */
        if (!(p_cur(p) == TOK_VOID && p_peek(p) == TOK_RPAREN)) {
            for (;;) {
                int pt;
                char *pname = NULL;
                as_ast *param;
                if (p_cur(p) == TOK_CONST) p_advance(p);
                pt = p_parse_type(p);
                if (pt < 0) {
                    p_fail(p, "expected a parameter type");
                    free(name); as_ast_free(params);
                    return NULL;
                }
                if (pt == AS_T_VOID) {
                    p_fail(p, "a parameter cannot have type void");
                    free(name); as_ast_free(params);
                    return NULL;
                }
                if (p_cur(p) == TOK_IDENT) {
                    pname = as_xstrdup(p->lx.cur.text);
                    p_advance(p);
                }
                param = as_ast_new(AST_PARAM, line);
                param->type = pt;
                param->name = pname;
                as_ast_addkid(params, param);
                if (p_cur(p) == TOK_COMMA) { p_advance(p); continue; }
                break;
            }
        } else {
            p_advance(p); /* consume the lone 'void' */
        }
    }
    if (!p_expect(p, TOK_RPAREN, "')'")) {
        free(name); as_ast_free(params);
        return NULL;
    }
    if (p_cur(p) != TOK_LBRACE) {
        p_fail(p, "a script function must have a body");
        free(name); as_ast_free(params);
        return NULL;
    }
    body = parse_block(p);
    if (!body) { free(name); as_ast_free(params); return NULL; }
    fn = as_ast_new(AST_FUNC, line);
    fn->type = ret;
    fn->name = name;
    as_ast_addkid(fn, params);
    as_ast_addkid(fn, body);
    return fn;
}

as_ast *as_parse_program(as_parser *p, const char *src, const char *section)
{
    as_ast *prog;
    as_lexer_init(&p->lx, src, section);
    if (p->lx.failed) {
        p->failed = 1;
        p->errline = p->lx.line;
        snprintf(p->errmsg, sizeof p->errmsg, "%s", p->lx.errmsg);
        return NULL;
    }
    prog = as_ast_new(AST_PROGRAM, 1);
    while (p_cur(p) != TOK_EOF && !p->failed) {
        as_ast *fn = parse_function(p);
        if (!fn) { as_ast_free(prog); return NULL; }
        as_ast_addkid(prog, fn);
    }
    if (p->lx.failed) {
        p->failed = 1;
        p->errline = p->lx.line;
        snprintf(p->errmsg, sizeof p->errmsg, "%s", p->lx.errmsg);
        as_ast_free(prog);
        return NULL;
    }
    return prog;
}

void as_parser_init(as_parser *p)
{
    memset(p, 0, sizeof *p);
}

void as_parser_free(as_parser *p)
{
    as_lexer_free(&p->lx);
}

/* ---------------- declaration strings ---------------- */
/* Parses "void print()", "float f(int, uint)", "float k(const float[] &in, uint)".
 * Parameter array/object types are recorded as AS_T_VOID with the
 * param_is_array flag (set in the as_decl via a parallel array). For the
 * subset we only need scalar params, but the decl must parse. */

int as_parse_decl(const char *decl, as_decl *out)
{
    as_parser p;
    int ret;
    char *name = NULL;
    memset(out, 0, sizeof *out);
    as_parser_init(&p);
    as_lexer_init(&p.lx, decl, "<decl>");
    if (p.lx.failed) {
        snprintf(out->errmsg, sizeof out->errmsg, "%s", p.lx.errmsg);
        out->failed = 1;
        as_parser_free(&p);
        return -1;
    }
    if (p_cur(&p) == TOK_CONST) p_advance(&p);
    ret = p_parse_type(&p);
    if (ret < 0) {
        snprintf(out->errmsg, sizeof out->errmsg, "expected a return type in declaration");
        out->failed = 1;
        as_parser_free(&p);
        return -1;
    }
    out->ret = ret;
    /* optional '&' on the return type is not supported in the subset */
    if (p_cur(&p) == TOK_BITAND) {
        snprintf(out->errmsg, sizeof out->errmsg, "reference return types are not supported in this subset");
        out->failed = 1;
        as_parser_free(&p);
        return -1;
    }
    if (p_cur(&p) != TOK_IDENT) {
        snprintf(out->errmsg, sizeof out->errmsg, "expected a function name in declaration");
        out->failed = 1;
        as_parser_free(&p);
        return -1;
    }
    name = as_xstrdup(p.lx.cur.text);
    p_advance(&p);
    if (p_cur(&p) == TOK_COLON) {
        snprintf(out->errmsg, sizeof out->errmsg, "scoped declarations (obj::method) are not supported in this subset");
        out->failed = 1;
        free(name);
        as_parser_free(&p);
        return -1;
    }
    if (!p_expect(&p, TOK_LPAREN, "'('")) {
        snprintf(out->errmsg, sizeof out->errmsg, "%s", p.errmsg);
        out->failed = 1;
        free(name);
        as_parser_free(&p);
        return -1;
    }
    out->name = name;
    if (p_cur(&p) != TOK_RPAREN) {
        if (!(p_cur(&p) == TOK_VOID && p_peek(&p) == TOK_RPAREN)) {
            int cap = 0;
            for (;;) {
                int pt, is_array = 0;
                if (p_cur(&p) == TOK_CONST) p_advance(&p);
                pt = p_parse_type(&p);
                if (pt < 0) {
                    snprintf(out->errmsg, sizeof out->errmsg, "expected a parameter type in declaration");
                    out->failed = 1;
                    as_parser_free(&p);
                    return -1;
                }
                if (p_cur(&p) == TOK_LBRACKET) {
                    /* array type, e.g. float[] */
                    p_advance(&p);
                    if (!p_expect(&p, TOK_RBRACKET, "']'")) {
                        snprintf(out->errmsg, sizeof out->errmsg, "%s", p.errmsg);
                        out->failed = 1;
                        as_parser_free(&p);
                        return -1;
                    }
                    is_array = 1;
                }
                if (p_cur(&p) == TOK_BITAND) {
                    p_advance(&p);
                    /* optional in/out/inout qualifier */
                    if (p_cur(&p) == TOK_IDENT) {
                        const char *q = p.lx.cur.text;
                        if (strcmp(q, "in") == 0 || strcmp(q, "out") == 0 ||
                            strcmp(q, "inout") == 0)
                            p_advance(&p);
                    }
                }
                /* optional parameter name */
                if (p_cur(&p) == TOK_IDENT) p_advance(&p);
                if (out->nparams >= cap) {
                    cap = cap ? cap * 2 : 4;
                    out->params = as_xrealloc(out->params, (size_t)cap * sizeof *out->params);
                }
                /* array params are recorded as void (unrepresentable) */
                out->params[out->nparams++] = is_array ? AS_T_VOID : pt;
                if (p_cur(&p) == TOK_COMMA) { p_advance(&p); continue; }
                break;
            }
        } else {
            p_advance(&p); /* lone 'void' */
        }
    }
    if (!p_expect(&p, TOK_RPAREN, "')'")) {
        snprintf(out->errmsg, sizeof out->errmsg, "%s", p.errmsg);
        out->failed = 1;
        as_parser_free(&p);
        return -1;
    }
    if (p_cur(&p) != TOK_EOF) {
        snprintf(out->errmsg, sizeof out->errmsg, "unexpected trailing input in declaration");
        out->failed = 1;
        as_parser_free(&p);
        return -1;
    }
    as_parser_free(&p);
    return 0;
}

void as_decl_free(as_decl *d)
{
    free(d->name);
    free(d->params);
    memset(d, 0, sizeof *d);
}
