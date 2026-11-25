// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// FILE: src/occam_frontend.c
#include "occam.h"
#include "gpu_ir.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static occam_proc_t *proc_new(occam_op_t op)
{
    occam_proc_t *p = calloc(1, sizeof(*p));
    if (p) p->op = op;
    return p;
}

/* Minimal recursive-descent skeleton for Occam subset:
   PROC name()
     PAR
       P1
       P2
     SEQ
       c ? x
       c ! y
   :
*/
static const char *skip_ws(const char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static int match(const char **ps, const char *kw)
{
    const char *s = skip_ws(*ps);
    size_t n = strlen(kw);
    if (strncmp(s, kw, n) == 0 && !isalnum((unsigned char)s[n])) {
        *ps = s + n;
        return 1;
    }
    return 0;
}

static occam_proc_t *parse_proc(const char **ps);

static occam_proc_t *parse_par(const char **ps)
{
    occam_proc_t *p = proc_new(OCC_PAR);
    if (!p) return NULL;
    p->children = calloc(OCCAM_MAX_PROCS, sizeof(occam_proc_t *));
    if (!p->children) { free(p); return NULL; }
    while (1) {
        const char *save = *ps;
        occam_proc_t *c = parse_proc(ps);
        if (!c) { *ps = save; break; }
        if (p->n_children >= OCCAM_MAX_PROCS) { free(c); break; }
        p->children[p->n_children++] = c;
    }
    return p;
}

static occam_proc_t *parse_seq(const char **ps)
{
    occam_proc_t *p = proc_new(OCC_SEQ);
    if (!p) return NULL;
    p->children = calloc(OCCAM_MAX_PROCS, sizeof(occam_proc_t *));
    if (!p->children) { free(p); return NULL; }
    while (1) {
        const char *save = *ps;
        occam_proc_t *c = parse_proc(ps);
        if (!c) { *ps = save; break; }
        if (p->n_children >= OCCAM_MAX_PROCS) { free(c); break; }
        p->children[p->n_children++] = c;
    }
    return p;
}

static occam_proc_t *parse_proc(const char **ps)
{
    const char *s = skip_ws(*ps);
    if (match(&s, "PAR") || match(&s, "par")) {
        *ps = s;
        return parse_par(ps);
    }
    if (match(&s, "SEQ") || match(&s, "seq")) {
        *ps = s;
        return parse_seq(ps);
    }
    if (match(&s, "SKIP") || match(&s, "skip")) {
        *ps = s;
        return proc_new(OCC_SKIP);
    }
    if (match(&s, "STOP") || match(&s, "stop")) {
        *ps = s;
        return proc_new(OCC_STOP);
    }
    /* channel input/output: c ? x or c ! e */
    if (isalpha((unsigned char)*s)) {
        occam_proc_t *p = proc_new(OCC_INPUT);
        if (!p) return NULL;
        /* simplified: treat identifier as channel op */
        while (isalnum((unsigned char)*s) || *s == '.') s++;
        s = skip_ws(s);
        if (*s == '?') {
            p->op = OCC_INPUT;
            s++;
        } else if (*s == '!') {
            p->op = OCC_OUTPUT;
            s++;
        } else {
            free(p);
            return NULL;
        }
        /* consume the operand variable (the x in "c ? x"): without
         * this the next parse step re-reads the bare variable as a
         * channel name, fails, and truncates the enclosing PAR/SEQ */
        s = skip_ws(s);
        while (isalnum((unsigned char)*s) || *s == '.') s++;
        *ps = s;
        return p;
    }
    return NULL;
}

occam_module_t *occam_parse(const char *src, size_t len)
{
    (void)len;
    occam_module_t *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    const char *p = src;
    p = skip_ws(p);
    if (match(&p, "PROC") || match(&p, "proc")) {
        p = skip_ws(p);
        while (isalnum((unsigned char)*p) || *p == '.') p++;
        p = skip_ws(p);
        if (*p == '(') {
            while (*p && *p != ')') p++;
            if (*p == ')') p++;
        }
        occam_proc_t *body = parse_proc(&p);
        if (body) {
            m->procs[0] = body;
            m->n_procs = 1;
            m->entry_id = 0;
        }
    }
    return m;
}

void occam_free(occam_module_t *m)
{
    if (!m) return;
    /* shallow free for skeleton */
    free(m);
}

int occam_validate_csp(const occam_module_t *m)
{
    if (!m || m->n_procs == 0) return -1;
    /* basic well-formedness */
    return 0;
}
