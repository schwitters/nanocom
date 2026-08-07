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
#include "nidl_codegen_wit.h"

#define MAX_INCLUDE_DIRS 64
#define NCOM_NIDLGEN_INCLUDE_ENV "NCOM_NIDLGEN_INCLUDE"
#define INCLUDE_ENV_BUF_SIZE 4096

static int push_include_dir(const char **include_dirs, int *n_include_dirs, const char *dir)
{
    if (!include_dirs || !n_include_dirs || !dir || dir[0] == '\0') {
        return 0;
    }
    if (*n_include_dirs >= MAX_INCLUDE_DIRS) {
        return 0;
    }
    include_dirs[(*n_include_dirs)++] = dir;
    return 1;
}

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
        fprintf(stderr, "env: %s=\"dir1:dir2\" (or ';' separated)\n", NCOM_NIDLGEN_INCLUDE_ENV);
        return 2;
    }

    /* Collect include directories from environment, then CLI overrides/additions. */
    const char *include_dirs[MAX_INCLUDE_DIRS];
    int n_include_dirs = 0;
    char include_env_buf[INCLUDE_ENV_BUF_SIZE];

    memset(include_env_buf, 0, sizeof(include_env_buf));

    {
        const char *include_env = getenv(NCOM_NIDLGEN_INCLUDE_ENV);
        if (include_env && include_env[0] != '\0') {
            size_t env_len = strlen(include_env);
            if (env_len >= sizeof(include_env_buf)) {
                fprintf(stderr, "nidlgen: %s exceeds %u bytes\n",
                        NCOM_NIDLGEN_INCLUDE_ENV, (unsigned)sizeof(include_env_buf) - 1u);
                return 2;
            }
            memcpy(include_env_buf, include_env, env_len + 1);
#ifdef _WIN32
            const char *delims = ";";
#else
            const char *delims = ":;";
#endif
            char *tok = strtok(include_env_buf, delims);
            while (tok) {
                if (!push_include_dir(include_dirs, &n_include_dirs, tok)) {
                    fprintf(stderr, "nidlgen: too many include directories\n");
                    return 2;
                }
                tok = strtok(NULL, delims);
            }
        }
    }

    for (int i = 3; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'I') {
            const char *dir = (argv[i][2] != '\0') ? &argv[i][2]
                              : (i + 1 < argc ? argv[++i] : NULL);
            if (!dir || dir[0] == '\0') {
                fprintf(stderr, "nidlgen: -I requires a non-empty directory path\n");
                return 2;
            }
            if (!push_include_dir(include_dirs, &n_include_dirs, dir)) {
                fprintf(stderr, "nidlgen: too many include directories\n");
                return 2;
            }
            continue;
        }
        fprintf(stderr, "nidlgen: unknown argument: %s\n", argv[i]);
        return 2;
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
        fprintf(stderr, "nidlgen: C codegen failed\n");
        arena_destroy(a);
        return 1;
    }

    if (!codegen_wit(a, ast, argv[2])) {
        fprintf(stderr, "nidlgen: WIT codegen failed\n");
        arena_destroy(a);
        return 1;
    }

    fprintf(stderr, "nidlgen: generated C headers into %s/include and WIT into %s/wit\n", argv[2], argv[2]);
    arena_destroy(a);
    return 0;
}
