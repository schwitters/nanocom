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
 * @file nidl_lex_re2c_wrap.c
 * @brief Nidl lex re2c wrap.
 */

#include "nidl_lex.h"
#include <stdlib.h>

token_t lexer_next_re2c(lexer_t *lx);

struct lexer_s {
    const char *s;
    const char *cur;
    uint32_t line;
    uint32_t col;
};

lexer_t *lexer_create(const char *src)
{
    lexer_t *lx = (lexer_t *)calloc(1, sizeof(*lx));
    if (!lx) return NULL;
    lx->s = src ? src : "";
    lx->cur = lx->s;
    lx->line = 1;
    lx->col = 1;
    return lx;
}

void lexer_destroy(lexer_t *lx)
{
    free(lx);
}

token_t lexer_next(lexer_t *lx)
{
    return lexer_next_re2c(lx);
}
