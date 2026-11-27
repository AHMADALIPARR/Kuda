// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* as_lexer.c - tokenizer for the C11 AngelScript-subset engine.
 * Keywords, identifiers, integer/float literals, string/char literals,
 * operators, punctuation, // and block comments. One token of lookahead.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "as_c11.h"

static const struct { const char *kw; as_tok_kind tok; } kKeywords[] = {
    { "void", TOK_VOID }, { "bool", TOK_BOOL },
    { "int", TOK_INT_KW }, { "int8", TOK_INT8 }, { "int16", TOK_INT16 },
    { "int64", TOK_INT64 },
    { "uint", TOK_UINT }, { "uint8", TOK_UINT8 }, { "uint16", TOK_UINT16 },
    { "uint32", TOK_UINT32 }, { "uint64", TOK_UINT64 },
    { "float", TOK_FLOAT_KW }, { "double", TOK_DOUBLE },
    { "true", TOK_TRUE }, { "false", TOK_FALSE },
    { "if", TOK_IF }, { "else", TOK_ELSE }, { "while", TOK_WHILE },
    { "for", TOK_FOR }, { "return", TOK_RETURN }, { "break", TOK_BREAK },
    { "continue", TOK_CONTINUE }, { "const", TOK_CONST },
};

const char *as_token_name(as_tok_kind k)
{
    switch (k) {
    case TOK_EOF: return "end of input";
    case TOK_IDENT: return "identifier";
    case TOK_INT: return "integer literal";
    case TOK_FLOAT: return "float literal";
    case TOK_STRING: return "string literal";
    case TOK_CHAR: return "character literal";
    default: return "token";
    }
}

static void tok_free(as_token *t)
{
    free(t->text);
    memset(t, 0, sizeof *t);
}

static void skip_ws_comments(as_lexer *lx)
{
    const char *s = lx->src;
    for (;;) {
        char c = s[lx->pos];
        if (c == ' ' || c == '\t' || c == '\r') { lx->pos++; continue; }
        if (c == '\n') { lx->pos++; lx->line++; continue; }
        if (c == '/' && s[lx->pos + 1] == '/') {
            while (s[lx->pos] && s[lx->pos] != '\n') lx->pos++;
            continue;
        }
        if (c == '/' && s[lx->pos + 1] == '*') {
            lx->pos += 2;
            while (s[lx->pos] && !(s[lx->pos] == '*' && s[lx->pos + 1] == '/')) {
                if (s[lx->pos] == '\n') lx->line++;
                lx->pos++;
            }
            if (s[lx->pos]) lx->pos += 2;
            continue;
        }
        break;
    }
}

static as_tok_kind keyword_of(const char *s, size_t n)
{
    size_t i;
    for (i = 0; i < sizeof kKeywords / sizeof kKeywords[0]; i++) {
        if (strlen(kKeywords[i].kw) == n &&
            strncmp(kKeywords[i].kw, s, n) == 0)
            return kKeywords[i].tok;
    }
    return TOK_IDENT;
}

/* Lex one token starting at lx->pos. Assumes whitespace already skipped. */
static void lex_one(as_lexer *lx, as_token *t)
{
    const char *s = lx->src;
    size_t start;
    memset(t, 0, sizeof *t);
    t->line = lx->line;
    skip_ws_comments(lx);
    t->line = lx->line;
    start = lx->pos;
    if (s[lx->pos] == '\0') { t->kind = TOK_EOF; return; }

    char c = s[lx->pos];

    /* Identifier / keyword */
    if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)s[lx->pos]) || s[lx->pos] == '_')
            lx->pos++;
        t->text = as_xstrndup(s + start, lx->pos - start);
        t->kind = keyword_of(t->text, lx->pos - start);
        return;
    }

    /* Number literal */
    if (isdigit((unsigned char)c) ||
        (c == '.' && isdigit((unsigned char)s[lx->pos + 1]))) {
        int is_float = 0, is_hex = 0;
        if (c == '0' && (s[lx->pos + 1] == 'x' || s[lx->pos + 1] == 'X')) {
            is_hex = 1;
            lx->pos += 2;
            while (isxdigit((unsigned char)s[lx->pos])) lx->pos++;
        } else {
            while (isdigit((unsigned char)s[lx->pos])) lx->pos++;
            if (s[lx->pos] == '.' && s[lx->pos + 1] != '.') {
                is_float = 1;
                lx->pos++;
                while (isdigit((unsigned char)s[lx->pos])) lx->pos++;
            }
            if (s[lx->pos] == 'e' || s[lx->pos] == 'E') {
                is_float = 1;
                lx->pos++;
                if (s[lx->pos] == '+' || s[lx->pos] == '-') lx->pos++;
                while (isdigit((unsigned char)s[lx->pos])) lx->pos++;
            }
        }
        /* suffixes: u, l, ll, f (any order, case-insensitive) */
        while (s[lx->pos] == 'u' || s[lx->pos] == 'U' ||
               s[lx->pos] == 'l' || s[lx->pos] == 'L' ||
               s[lx->pos] == 'f' || s[lx->pos] == 'F') {
            char sc = s[lx->pos];
            if (sc == 'u' || sc == 'U') t->is_unsigned = 1;
            else if (sc == 'l' || sc == 'L') t->is_long = 1;
            else if (sc == 'f' || sc == 'F') { t->is_float_suffix = 1; is_float = 1; }
            lx->pos++;
        }
        t->text = as_xstrndup(s + start, lx->pos - start);
        t->is_float = is_float;
        if (is_float) {
            t->kind = TOK_FLOAT;
            t->fval = strtod(t->text, NULL);
        } else {
            t->kind = TOK_INT;
            if (is_hex) {
                t->ival = (int64_t)strtoull(t->text, NULL, 16);
                /* hex literals are unsigned unless they fit; keep raw bits */
                if (!t->is_unsigned && t->ival < 0) t->is_unsigned = 1;
            } else {
                if (t->is_unsigned)
                    t->ival = (int64_t)strtoull(t->text, NULL, 10);
                else
                    t->ival = strtoll(t->text, NULL, 10);
            }
        }
        return;
    }

    /* String literal */
    if (c == '"') {
        lx->pos++;
        size_t cap = 16, n = 0;
        char *buf = as_xmalloc(cap);
        while (s[lx->pos] && s[lx->pos] != '"') {
            char ch = s[lx->pos];
            if (ch == '\\' && s[lx->pos + 1]) {
                lx->pos++;
                switch (s[lx->pos]) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case 'r': ch = '\r'; break;
                case '\\': ch = '\\'; break;
                case '"': ch = '"'; break;
                case '\'': ch = '\''; break;
                case '0': ch = '\0'; break;
                default: ch = s[lx->pos]; break;
                }
            }
            if (ch == '\n') lx->line++;
            if (n + 1 >= cap) { cap *= 2; buf = as_xrealloc(buf, cap); }
            buf[n++] = ch;
            lx->pos++;
        }
        if (s[lx->pos] == '"') lx->pos++;
        else {
            snprintf(lx->errmsg, sizeof lx->errmsg,
                     "unterminated string literal");
            lx->failed = 1;
        }
        buf[n] = '\0';
        t->kind = TOK_STRING;
        t->text = buf;
        return;
    }

    /* Character literal */
    if (c == '\'') {
        lx->pos++;
        int v = 0;
        if (s[lx->pos] == '\\' && s[lx->pos + 1]) {
            lx->pos++;
            switch (s[lx->pos]) {
            case 'n': v = '\n'; break;
            case 't': v = '\t'; break;
            case 'r': v = '\r'; break;
            case '\\': v = '\\'; break;
            case '\'': v = '\''; break;
            case '0': v = 0; break;
            default: v = (unsigned char)s[lx->pos]; break;
            }
            lx->pos++;
        } else if (s[lx->pos] && s[lx->pos] != '\'') {
            v = (unsigned char)s[lx->pos];
            lx->pos++;
        }
        if (s[lx->pos] == '\'') lx->pos++;
        t->kind = TOK_CHAR;
        t->ival = v;
        t->text = as_xstrndup(s + start, lx->pos - start);
        return;
    }

    /* Operators / punctuation (longest match first) */
    lx->pos++;
    switch (c) {
    case '+':
        if (s[lx->pos] == '+') { lx->pos++; t->kind = TOK_INC; }
        else if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_PLUS_ASSIGN; }
        else t->kind = TOK_PLUS;
        return;
    case '-':
        if (s[lx->pos] == '-') { lx->pos++; t->kind = TOK_DEC; }
        else if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_MINUS_ASSIGN; }
        else t->kind = TOK_MINUS;
        return;
    case '*':
        if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_STAR_ASSIGN; }
        else t->kind = TOK_STAR;
        return;
    case '/':
        if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_SLASH_ASSIGN; }
        else t->kind = TOK_SLASH;
        return;
    case '%':
        if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_PERCENT_ASSIGN; }
        else t->kind = TOK_PERCENT;
        return;
    case '=':
        if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_EQ; }
        else t->kind = TOK_ASSIGN;
        return;
    case '!':
        if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_NE; }
        else t->kind = TOK_NOT;
        return;
    case '<':
        if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_LE; }
        else if (s[lx->pos] == '<') {
            lx->pos++;
            if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_SHL_ASSIGN; }
            else t->kind = TOK_SHL;
        } else t->kind = TOK_LT;
        return;
    case '>':
        if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_GE; }
        else if (s[lx->pos] == '>') {
            lx->pos++;
            if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_SHR_ASSIGN; }
            else t->kind = TOK_SHR;
        } else t->kind = TOK_GT;
        return;
    case '&':
        if (s[lx->pos] == '&') { lx->pos++; t->kind = TOK_AND; }
        else if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_BITAND_ASSIGN; }
        else t->kind = TOK_BITAND;
        return;
    case '|':
        if (s[lx->pos] == '|') { lx->pos++; t->kind = TOK_OR; }
        else if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_BITOR_ASSIGN; }
        else t->kind = TOK_BITOR;
        return;
    case '^':
        if (s[lx->pos] == '=') { lx->pos++; t->kind = TOK_BITXOR_ASSIGN; }
        else t->kind = TOK_BITXOR;
        return;
    case '~': t->kind = TOK_BITNOT; return;
    case '(': t->kind = TOK_LPAREN; return;
    case ')': t->kind = TOK_RPAREN; return;
    case '{': t->kind = TOK_LBRACE; return;
    case '}': t->kind = TOK_RBRACE; return;
    case '[': t->kind = TOK_LBRACKET; return;
    case ']': t->kind = TOK_RBRACKET; return;
    case ';': t->kind = TOK_SEMI; return;
    case ',': t->kind = TOK_COMMA; return;
    case ':': t->kind = TOK_COLON; return;
    case '?': t->kind = TOK_QUESTION; return;
    case '.': t->kind = TOK_DOT; return;
    default:
        snprintf(lx->errmsg, sizeof lx->errmsg,
                 "unexpected character '%c'", c);
        lx->failed = 1;
        t->kind = TOK_EOF;
        return;
    }
}

void as_lexer_init(as_lexer *lx, const char *src, const char *section)
{
    memset(lx, 0, sizeof *lx);
    lx->src = src;
    lx->section = section;
    lx->pos = 0;
    lx->line = 1;
    memset(&lx->cur, 0, sizeof lx->cur);
    memset(&lx->peek, 0, sizeof lx->peek);
    lex_one(lx, &lx->cur);
    lex_one(lx, &lx->peek);
}

void as_lexer_next(as_lexer *lx)
{
    tok_free(&lx->cur);
    lx->cur = lx->peek;
    memset(&lx->peek, 0, sizeof lx->peek);
    lex_one(lx, &lx->peek);
}

void as_lexer_free(as_lexer *lx)
{
    tok_free(&lx->cur);
    tok_free(&lx->peek);
}

/* Shared allocation helpers (used across the engine). */
void *as_xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) abort();
    return p;
}

void *as_xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) abort();
    return q;
}

char *as_xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = as_xmalloc(n);
    memcpy(p, s, n);
    return p;
}

char *as_xstrndup(const char *s, size_t n)
{
    char *p = as_xmalloc(n + 1);
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}
