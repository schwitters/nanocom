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
 * @file main.c
 * @brief Main.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nidl_arena.h"
#include "nidl_parse.h"
#include "nidl_codegen_c.h"

#define MAX_INCLUDE_DIRS 64

static char *read_all(arena_t *a,const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (n < 0) { fclose(fp); return NULL; }
    char *buf = (char *)arena_calloc(a,(size_t)n + 1, 1);
    if (!buf) { fclose(fp); return NULL; }
    if (fread(buf, 1, (size_t)n, fp) != (size_t)n) { fclose(fp); return NULL; } /* buf is arena-owned; caller destroys arena */
    fclose(fp);
    buf[n] = '\0';
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: nidlgen <input.idl> <out_dir> [-I<dir>...]\n");
        return 2;
    }

    /* Collect -I include directories from optional extra arguments. */
    const char *include_dirs[MAX_INCLUDE_DIRS];
    int n_include_dirs = 0;
    for (int i = 3; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'I') {
            const char *dir = (argv[i][2] != '\0') ? &argv[i][2]
                              : (i + 1 < argc ? argv[++i] : NULL);
            if (dir && n_include_dirs < MAX_INCLUDE_DIRS)
                include_dirs[n_include_dirs++] = dir;
        }
    }

    /* Build the import context; push the root file for cycle detection. */
    nidl_include_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.include_dirs   = include_dirs;
    ctx.n_include_dirs = n_include_dirs;
    ctx.stack[ctx.stack_depth++] = argv[1];

    arena_t *a = arena_create();
    char *src = read_all(a, argv[1]);
    if (!src) {
        fprintf(stderr, "nidlgen: failed to read %s\n", argv[1]);
        arena_destroy(a);
        return 1;
    }

    idl_file_t *ast = nidl_parse(a, src, argv[1], &ctx);
    if (!ast) {
        fprintf(stderr, "nidlgen: parse failed\n");
        arena_destroy(a);
        return 1;
    }

    if (!codegen_c_headers(a, ast, argv[2])) {
        fprintf(stderr, "nidlgen: codegen failed\n");
        arena_destroy(a);
        return 1;
    }

    fprintf(stderr, "nidlgen: generated C headers into %s/include\n", argv[2]);
    arena_destroy(a);
    return 0;
}
