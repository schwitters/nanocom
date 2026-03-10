/*
 * Copyright 2026 nano_com authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * @file nidl_parse.c
 * @brief Nidl parse.
 */

#include "nidl_parse.h"
#include "nidl_lex.h"
#include "nidl_arena.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef struct parser_s {
    lexer_t *lx;
    token_t t;
    arena_t *a;
    idl_file_t *file;
    int had_error;
    const char *src_path;       /* path to the file being parsed (for relative import resolution) */
    nidl_include_ctx_t *ctx;    /* import resolution context */
} parser_t;

/* ---------- import helpers ------------------------------------------- */

/* Read an entire file into a NUL-terminated arena-allocated buffer. */
static char *read_file_src(arena_t *a, const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NULL; }
    long n = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0 || n < 0) { fclose(fp); return NULL; }
    char *buf = (char *)arena_calloc(a, (size_t)n + 1, 1);
    if (!buf) { fclose(fp); return NULL; }
    if (fread(buf, 1, (size_t)n, fp) != (size_t)n) { fclose(fp); return NULL; }
    fclose(fp);
    buf[n] = '\0';
    return buf;
}

/* Resolve an import path to an existing file path (arena-allocated result).
 * Search order: (1) relative to directory of src_path, (2) each include_dir. */
static const char *resolve_import_path(arena_t *a,
                                       const char *src_path,
                                       const char *import_path,
                                       const char **include_dirs,
                                       int n_include_dirs)
{
    struct stat sb;

    /* Absolute path: use as-is. */
    if (import_path[0] == '/' || import_path[0] == '\\') {
        if (stat(import_path, &sb) == 0)
            return arena_strdup(a, import_path, strlen(import_path));
        return NULL;
    }

    /* Relative to directory of the importing file. */
    if (src_path) {
        const char *last_sep = NULL;
        for (const char *p = src_path; *p; p++)
            if (*p == '/' || *p == '\\') last_sep = p;

        if (last_sep) {
            size_t dir_len = (size_t)(last_sep - src_path + 1);
            size_t imp_len = strlen(import_path);
            char *candidate = (char *)arena_alloc(a, dir_len + imp_len + 1);
            if (!candidate) return NULL;
            memcpy(candidate, src_path, dir_len);
            memcpy(candidate + dir_len, import_path, imp_len + 1);
            if (stat(candidate, &sb) == 0) return candidate;
        } else {
            /* src_path has no directory separator; try CWD-relative. */
            if (stat(import_path, &sb) == 0)
                return arena_strdup(a, import_path, strlen(import_path));
        }
    }

    /* Search include directories. */
    for (int i = 0; i < n_include_dirs; i++) {
        size_t dir_len = strlen(include_dirs[i]);
        size_t imp_len = strlen(import_path);
        size_t total   = dir_len + 1 + imp_len + 1;
        char *candidate = (char *)arena_alloc(a, total);
        if (!candidate) return NULL;
        snprintf(candidate, total, "%s/%s", include_dirs[i], import_path);
        if (stat(candidate, &sb) == 0) return candidate;
    }

    return NULL;
}

static void next(parser_t *p) { p->t = lexer_next(p->lx); }

static int is(parser_t *p, tok_kind_t k) { return p->t.kind == k; }

static void err(parser_t *p, const char *msg)
{
    fprintf(stderr, "IDL parse error at %u:%u: %s near '%.*s'\n", p->t.line, p->t.col, msg, (int)p->t.len, p->t.ptr);
    p->had_error = 1;
}

static int expect(parser_t *p, tok_kind_t k, const char *msg)
{
    if (!is(p, k)) { err(p, msg); return 0; }
    next(p);
    return 1;
}

static const char *tok_str(parser_t *p)
{
    return arena_strdup(p->a, p->t.ptr, p->t.len);
}


static const char *parse_doc_opt(parser_t *p)
{
    if (is(p, TOK_DOC)) {
        const char *d = tok_str(p);
        next(p);
        return d;
    }
    return NULL;
}


/* attribute list: [uuid("..")] or [optional] - only what we need */
static const char *parse_uuid_attr(parser_t *p)
{
    const char *uuid = NULL;
    if (!is(p, TOK_LBRACKET)) return NULL;

    next(p); /* '[' */
    if (!is(p, TOK_IDENT) || !(p->t.len==4 && memcmp(p->t.ptr,"uuid",4)==0)) {
        /* skip unknown attrs */
        while (!is(p, TOK_RBRACKET) && !is(p, TOK_EOF)) next(p);
        expect(p, TOK_RBRACKET, "expected ']'");
        return NULL;
    }
    next(p); /* uuid */
    if (!expect(p, TOK_LPAREN, "expected '(' after uuid")) return NULL;
    if (!is(p, TOK_STRING)) { err(p, "expected UUID string literal"); return NULL; }
    uuid = tok_str(p);
    next(p);
    (void)expect(p, TOK_RPAREN, "expected ')'");
    (void)expect(p, TOK_RBRACKET, "expected ']'");
    return uuid;
}

static int parse_optional_attr(parser_t *p)
{
    int opt = 0;
    if (!is(p, TOK_LBRACKET)) return 0;
    next(p);
    if (is(p, TOK_IDENT) && p->t.len==8 && memcmp(p->t.ptr,"optional",8)==0) {
        opt = 1;
        next(p);
    } else {
        /* skip unknown */
        while (!is(p, TOK_RBRACKET) && !is(p, TOK_EOF)) next(p);
    }
    (void)expect(p, TOK_RBRACKET, "expected ']'");
    return opt;
}

static void append_typedef(idl_file_t *f, idl_typedef_t *td)
{
    td->next = f->typedefs;
    f->typedefs = td;
}

static void append_struct(idl_file_t *f, idl_struct_t *st)
{
    st->next = f->structs;
    f->structs = st;
}

static void append_interface(idl_file_t *f, idl_interface_t *it)
{
    it->next = f->interfaces;
    f->interfaces = it;
}


static void append_coclass(idl_file_t *f, idl_coclass_t *cc)
{
    cc->next = f->coclasses;
    f->coclasses = cc;
}

/* Forward declaration: nidl_parse is called recursively for imports. */
static idl_file_t *nidl_parse_internal(arena_t *a, const char *src,
                                       const char *src_path, nidl_include_ctx_t *ctx);

/* Parse: import "path"; — resolves, reads, and merges an imported IDL file. */
static void parse_import(parser_t *p)
{
    next(p); /* consume 'import' */
    if (!is(p, TOK_STRING)) { err(p, "expected import path string"); return; }
    const char *import_path = tok_str(p);
    next(p);
    (void)expect(p, TOK_SEMI, "expected ';' after import path");
    if (p->had_error) return;

    /* Resolve the path. */
    const char **dirs = p->ctx ? p->ctx->include_dirs : NULL;
    int ndirs         = p->ctx ? p->ctx->n_include_dirs : 0;
    const char *resolved = resolve_import_path(p->a, p->src_path, import_path, dirs, ndirs);
    if (!resolved) {
        fprintf(stderr, "IDL import error at %u:%u: cannot find '%s'\n",
                p->t.line, p->t.col, import_path);
        p->had_error = 1;
        return;
    }

    /* Cycle detection. */
    if (p->ctx) {
        for (int i = 0; i < p->ctx->stack_depth; i++) {
            if (strcmp(p->ctx->stack[i], resolved) == 0) {
                fprintf(stderr, "IDL import error: circular import '%s'\n", resolved);
                p->had_error = 1;
                return;
            }
        }
    }

    /* Read imported file. */
    char *imp_src = read_file_src(p->a, resolved);
    if (!imp_src) {
        fprintf(stderr, "IDL import error: cannot read '%s'\n", resolved);
        p->had_error = 1;
        return;
    }

    /* Push resolved path onto cycle-detection stack, parse, pop. */
    if (p->ctx && p->ctx->stack_depth < 32)
        p->ctx->stack[p->ctx->stack_depth++] = resolved;

    idl_file_t *imp_file = nidl_parse_internal(p->a, imp_src, resolved, p->ctx);

    if (p->ctx && p->ctx->stack_depth > 0)
        p->ctx->stack_depth--;

    if (!imp_file) {
        fprintf(stderr, "IDL import error: failed to parse '%s'\n", resolved);
        p->had_error = 1;
        return;
    }

    /* Merge imported interfaces (deduplicated, marked is_imported). */
    for (idl_interface_t *it = imp_file->interfaces; it; ) {
        idl_interface_t *next_it = it->next;
        int found = 0;
        for (idl_interface_t *ex = p->file->interfaces; ex; ex = ex->next) {
            if (ex->name && it->name && strcmp(ex->name, it->name) == 0) {
                found = 1; break;
            }
        }
        if (!found) {
            it->is_imported    = 1;
            it->source_module  = imp_file->module_name;
            it->next           = p->file->interfaces;
            p->file->interfaces = it;
        }
        it = next_it;
    }

    /* Merge imported structs (deduplicated). */
    for (idl_struct_t *st = imp_file->structs; st; ) {
        idl_struct_t *next_st = st->next;
        int found = 0;
        for (idl_struct_t *ex = p->file->structs; ex; ex = ex->next) {
            if (ex->name && st->name && strcmp(ex->name, st->name) == 0) {
                found = 1; break;
            }
        }
        if (!found) {
            st->is_imported   = 1;
            st->source_module = imp_file->module_name;
            st->next          = p->file->structs;
            p->file->structs  = st;
        }
        st = next_st;
    }

    /* Record the import for codegen (#include emission). */
    idl_import_t *rec = (idl_import_t *)arena_alloc(p->a, sizeof(*rec));
    rec->path        = import_path;
    rec->module_name = imp_file->module_name;
    rec->next        = p->file->imports;
    p->file->imports = rec;
}

/* Parse: [uuid(...)] coclass name { };  (contents ignored for now) */
static void parse_coclass(parser_t *p, const char *uuid, const char *doc)
{
    next(p); /* coclass */
    if (!is(p, TOK_IDENT)) { err(p, "expected coclass name"); return; }
    const char *name = tok_str(p); next(p);
    (void)expect(p, TOK_LBRACE, "expected '{' after coclass name");
    while (!is(p, TOK_RBRACE) && !is(p, TOK_EOF)) next(p);
    (void)expect(p, TOK_RBRACE, "expected '}' after coclass");
    (void)expect(p, TOK_SEMI, "expected ';' after coclass");

    idl_coclass_t *cc = (idl_coclass_t *)arena_alloc(p->a, sizeof(*cc));
    cc->doc = doc;
    cc->name = name;
    cc->uuid = uuid;
    append_coclass(p->file, cc);
}
static void append_method(idl_interface_t *it, idl_method_t *m)
{
    m->next = it->methods;
    it->methods = m;
}

static void append_param(idl_method_t *m, idl_param_t *p2)
{
    p2->next = m->params;
    m->params = p2;
}

static void append_field(idl_struct_t *st, idl_struct_field_t *f2)
{
    f2->next = st->fields;
    st->fields = f2;
}


static const char *parse_type_spec(parser_t *p)
{
    const char *qual = NULL;
    const char *sign = NULL;

    if (is(p, TOK_CONST)) { qual = "const"; next(p); }

    if (is(p, TOK_UNSIGNED)) { sign = "unsigned"; next(p); }
    else if (is(p, TOK_SIGNED)) { sign = "signed"; next(p); }

    const char *base = NULL;
    if (is(p, TOK_LONG)) {
        next(p);
        if (is(p, TOK_LONG)) { next(p); base = "long long"; }
        else base = "long";
    } else if (is(p, TOK_SHORT)) {
        next(p); base = "short";
    } else if (is(p, TOK_INT_KW)) {
        next(p); base = "int";
    } else if (is(p, TOK_CHAR_KW)) {
        next(p); base = "char";
    } else if (is(p, TOK_OCTET)) {
        next(p); base = "octet";
    } else if (is(p, TOK_IDENT)) {
        base = tok_str(p); next(p);
    } else {
        err(p, "expected type specifier");
        return NULL;
    }

    int ptrs = 0;
    while (is(p, TOK_STAR)) { next(p); ptrs++; }

    char buf[256];
    snprintf(buf, sizeof(buf), "%s%s%s%s%s",
             qual  ? qual  : "", qual  ? " " : "",
             sign  ? sign  : "", sign  ? " " : "",
             base);
    /* append pointer stars safely */
    for (int i = 0; i < ptrs; i++) {
        size_t len = strlen(buf);
        if (len + 3 >= sizeof(buf)) break;
        buf[len] = ' '; buf[len+1] = '*'; buf[len+2] = '\0';
    }

    return arena_strdup(p->a, buf, strlen(buf));
}

/* Parse: typedef <type> <alias>; */
static void parse_typedef(parser_t *p, const char *doc)
{
    next(p); /* typedef */
    const char *target = parse_type_spec(p);
    if (!target) return;

    if (!is(p, TOK_IDENT)) { err(p, "expected alias after typedef"); return; }
    const char *alias = tok_str(p);
    next(p);
    (void)expect(p, TOK_SEMI, "expected ';' after typedef");

    idl_typedef_t *td = (idl_typedef_t *)arena_alloc(p->a, sizeof(*td));
    td->doc = doc;
    td->alias = alias;
    td->target = target;
    append_typedef(p->file, td);
}

/* Parse: struct name { <type> <name>; ... }; */
static void parse_struct(parser_t *p, const char *doc)
{
    next(p); /* struct */
    if (!is(p, TOK_IDENT)) { err(p, "expected struct name"); return; }
    const char *name = tok_str(p);
    next(p);
    (void)expect(p, TOK_LBRACE, "expected '{' after struct name");

    idl_struct_t *st = (idl_struct_t *)arena_alloc(p->a, sizeof(*st));
    st->doc = doc;
    st->name = name;
    append_struct(p->file, st);

    while (!is(p, TOK_RBRACE) && !is(p, TOK_EOF)) {
        const char *fdoc = parse_doc_opt(p);
        const char *ft = parse_type_spec(p);
        if (!ft) return;

        if (!is(p, TOK_IDENT)) { err(p, "expected field name"); return; }
        const char *fn = tok_str(p); next(p);
        (void)expect(p, TOK_SEMI, "expected ';' after field");
        idl_struct_field_t *f2 = (idl_struct_field_t *)arena_alloc(p->a, sizeof(*f2));
        f2->doc = fdoc;
        f2->type = ft; f2->name = fn;
        append_field(st, f2);
    }
    (void)expect(p, TOK_RBRACE, "expected '}' after struct");
    (void)expect(p, TOK_SEMI, "expected ';' after struct");
}

/* Parse a param: ([optional]) <dir> <type> <name> */
static idl_param_t *parse_param(parser_t *p)
{
    const char *doc = parse_doc_opt(p);
    int optional = 0;
    if (is(p, TOK_LBRACKET)) optional = parse_optional_attr(p);

    const char *dir = NULL;
    if (is(p, TOK_IN)) { dir = "in"; next(p); }
    else if (is(p, TOK_OUT)) { dir = "out"; next(p); }
    else if (is(p, TOK_INOUT)) { dir = "inout"; next(p); }
    else { err(p, "expected in|out|inout"); return NULL; }

    const char *type = parse_type_spec(p);
    if (!type) return NULL;


    if (!is(p, TOK_IDENT)) { err(p, "expected parameter name"); return NULL; }
    const char *name = tok_str(p); next(p);

    idl_param_t *pa = (idl_param_t *)arena_alloc(p->a, sizeof(*pa));
    pa->doc = doc;
    pa->dir = dir; pa->type = type; pa->name = name; pa->optional = optional;
    return pa;
}

/* Parse method: <ret_type> <name>( ... ); */
static void parse_method(parser_t *p, idl_interface_t *it, const char *doc)
{
    if (!is(p, TOK_IDENT) && p->t.kind != TOK_TYPEDEF) { /* ret_type ident */
        err(p, "expected return type"); return;
    }
    const char *ret = parse_type_spec(p);
    if (!ret) return;

    if (!is(p, TOK_IDENT)) { err(p, "expected method name"); return; }
    const char *name = tok_str(p); next(p);
    (void)expect(p, TOK_LPAREN, "expected '(' after method name");

    idl_method_t *m = (idl_method_t *)arena_alloc(p->a, sizeof(*m));
    m->doc = doc;
    m->ret_type = ret;
    m->name = name;
    append_method(it, m);

    if (!is(p, TOK_RPAREN)) {
        for (;;) {
            idl_param_t *pa = parse_param(p);
            if (!pa) return;
            append_param(m, pa);
            if (is(p, TOK_COMMA)) { next(p); continue; }
            break;
        }
    }
    (void)expect(p, TOK_RPAREN, "expected ')' after params");
    (void)expect(p, TOK_SEMI, "expected ';' after method");
}

/* Parse: [uuid(...)] interface name (: base)? { methods } ; */
static void parse_interface(parser_t *p, const char *uuid, const char *doc)
{
    next(p); /* interface */
    if (!is(p, TOK_IDENT)) { err(p, "expected interface name"); return; }
    const char *name = tok_str(p); next(p);

    const char *base = NULL;
    if (is(p, TOK_COLON)) {
        next(p);
        if (!is(p, TOK_IDENT)) { err(p, "expected base interface name"); return; }
        base = tok_str(p); next(p);
    }

    (void)expect(p, TOK_LBRACE, "expected '{' after interface");

    idl_interface_t *it = (idl_interface_t *)arena_alloc(p->a, sizeof(*it));
    it->doc = doc;
    it->name = name;
    it->base = base;
    it->uuid = uuid;
    append_interface(p->file, it);

    while (!is(p, TOK_RBRACE) && !is(p, TOK_EOF)) {
        const char *mdoc = parse_doc_opt(p);
        parse_method(p, it, mdoc);
        if (p->had_error) return;
    }
    (void)expect(p, TOK_RBRACE, "expected '}' after interface");
    (void)expect(p, TOK_SEMI, "expected ';' after interface");
}

static idl_file_t *nidl_parse_internal(arena_t *a, const char *src,
                                       const char *src_path, nidl_include_ctx_t *ctx)
{
    parser_t p;
    memset(&p, 0, sizeof(p));
    p.a        = a;
    p.src_path = src_path;
    p.ctx      = ctx;
    p.lx       = lexer_create(src);
    p.file     = (idl_file_t *)arena_alloc(a, sizeof(idl_file_t));
    next(&p);

    /* import "path"; statements before the module block */
    while (is(&p, TOK_IMPORT) && !p.had_error)
        parse_import(&p);
    if (p.had_error) goto fail;

    /* module <name> { ... }; */
    if (!is(&p, TOK_MODULE)) { err(&p, "expected 'module'"); goto fail; }
    next(&p);
    if (!is(&p, TOK_IDENT)) { err(&p, "expected module name"); goto fail; }
    p.file->module_name = tok_str(&p);
    next(&p);
    if (!expect(&p, TOK_LBRACE, "expected '{' after module")) goto fail;

    const char *pending_doc = NULL;

    while (!is(&p, TOK_RBRACE) && !is(&p, TOK_EOF) && !p.had_error) {
        if (!pending_doc) pending_doc = parse_doc_opt(&p);
        if (is(&p, TOK_TYPEDEF)) {
            parse_typedef(&p, pending_doc);
            pending_doc = NULL;
            continue;
        }
        if (is(&p, TOK_STRUCT)) {
            parse_struct(&p, pending_doc);
            pending_doc = NULL;
            continue;
        }
        const char *uuid = NULL;
        if (is(&p, TOK_LBRACKET)) {
            uuid = parse_uuid_attr(&p);
            if (p.had_error) break;
        }
        if (is(&p, TOK_INTERFACE)) {
            parse_interface(&p, uuid, pending_doc);
            pending_doc = NULL;
            continue;
        }
        if (is(&p, TOK_COCLASS)) {
            parse_coclass(&p, uuid, pending_doc);
            pending_doc = NULL;
            continue;
        }
        err(&p, "unexpected token in module");
        break;
    }

    if (!p.had_error) {
        (void)expect(&p, TOK_RBRACE, "expected '}' at end of module");
        (void)expect(&p, TOK_SEMI, "expected ';' after module");
    }

    lexer_destroy(p.lx);
    p.lx = NULL; /* prevent double-free if had_error path is taken */

    if (p.had_error) goto fail;

    /* NOTE: arena is intentionally kept alive with the AST; the caller owns it. */
    return p.file;

fail:
    lexer_destroy(p.lx); /* safe: free(NULL) is a no-op */
    /* Do NOT call arena_destroy — the arena is owned by the caller. */
    return NULL;
}

idl_file_t *nidl_parse(arena_t *a, const char *src,
                       const char *src_path, nidl_include_ctx_t *ctx)
{
    return nidl_parse_internal(a, src, src_path, ctx);
}
