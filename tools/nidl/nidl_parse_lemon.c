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
 * @file nidl_parse_lemon.c
 * @brief Nidl parse lemon.
 */

#include "nidl_parse.h"
#include "nidl_lex.h"
#include "nidl_arena.h"
#include "nidl_ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Lemon-generated parser API (names depend on %name; default is "Parse").
   We rely on lemon defaults: ParseAlloc/Parse/ParseFree. */
void *ParseAlloc(void *(*mallocProc)(size_t));
void Parse(void *pParser, int tokenId, token_t token, void *ctx);
void ParseFree(void *pParser, void (*freeProc)(void*));

/* Token id mapping:
   lemon uses integer token codes; with %token_prefix TOK_ and our enum,
   we can pass token.kind directly. */
typedef struct parse_ctx_s {
    arena_t *a;
    idl_file_t *file;
    int had_error;
} parse_ctx_t;

/* NOTE: import "..." is not yet supported in the re2c/lemon code path. */
idl_file_t *nidl_parse(arena_t *arena, const char *src,
                       const char *src_path, nidl_include_ctx_t *ctx)
{
    (void)src_path; (void)ctx;
    arena_t *a = arena ? arena : arena_create();
    if (!a) return NULL;

    parse_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.a = a;
    ctx.file = (idl_file_t *)arena_alloc(a, sizeof(idl_file_t));
    if (!ctx.file) { arena_destroy(a); return NULL; }

    lexer_t *lx = lexer_create(src);
    if (!lx) { arena_destroy(a); return NULL; }

    void *p = ParseAlloc(malloc);
    if (!p) { lexer_destroy(lx); arena_destroy(a); return NULL; }

    for (;;) {
        token_t t = lexer_next(lx);
        /* Lemon expects token code 0 for end-of-input (TOK_EOF is 0). */
        Parse(p, (int)t.kind, t, &ctx);
        if (t.kind == TOK_EOF) break;
        if (ctx.had_error) break;
    }

    ParseFree(p, free);
    lexer_destroy(lx);

    if (ctx.had_error) {
        arena_destroy(a);
        return NULL;
    }

    /* Note: arena ownership is tied to process lifetime in nidlgen.
       If you need to free it, extend API to return arena handle. */
    return ctx.file;
}
