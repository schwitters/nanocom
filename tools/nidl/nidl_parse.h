#ifndef NCOM_TOOLS_NIDL_NIDL_PARSE_H
#define NCOM_TOOLS_NIDL_NIDL_PARSE_H

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
#include "nidl_arena.h"
#include "nidl_ast.h"

/**
 * Context for resolving import paths and detecting circular imports.
 * Zero-initialize before first use (memset or = {0}).
 */
typedef struct nidl_include_ctx_s {
    const char **include_dirs;  /* additional search directories (from -I flags) */
    int n_include_dirs;
    const char *stack[32];      /* file paths currently being parsed (cycle detection) */
    int stack_depth;
} nidl_include_ctx_t;

/**
 * Parse IDL source into an AST.
 *
 * @param arena     Arena for all allocations; owned by the caller.
 * @param src       NUL-terminated IDL source text.
 * @param src_path  Path to the source file; used to resolve relative imports.
 *                  May be NULL if no imports are expected.
 * @param ctx       Import resolution context.  May be NULL.
 *
 * Returns NULL on error; errors are printed to stderr.
 */
idl_file_t *nidl_parse(arena_t *arena, const char *src,
                       const char *src_path, nidl_include_ctx_t *ctx);

#endif /* NCOM_TOOLS_NIDL_NIDL_PARSE_H */
