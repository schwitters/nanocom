#ifndef NCOM_TOOLS_NIDL_NIDL_LEX_H
#define NCOM_TOOLS_NIDL_NIDL_LEX_H

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
#include <stdint.h>

typedef int tok_kind_t;

#define TOK_EOF 0

#if defined(NANO_IDL_USE_RE2C_LEMON)
  #include "nidl_parser.h"
#else
  #define TOK_MODULE     1
  #define TOK_IDENT      2
  #define TOK_LBRACE     3
  #define TOK_RBRACE     4
  #define TOK_SEMI       5
  #define TOK_TYPEDEF    6
  #define TOK_STRUCT     7
  #define TOK_LBRACKET   8
  #define TOK_LPAREN     9
  #define TOK_STRING     10
  #define TOK_RPAREN     11
  #define TOK_RBRACKET   12
  #define TOK_INTERFACE  13
  #define TOK_COCLASS    14
  #define TOK_COLON      15
  #define TOK_COMMA      16
  #define TOK_IN         17
  #define TOK_OUT        18
  #define TOK_INOUT      19
#define TOK_CONST      20
#define TOK_UNSIGNED   21
#define TOK_SIGNED     22
#define TOK_LONG       23
#define TOK_SHORT      24
#define TOK_INT_KW     25
#define TOK_CHAR_KW    26
#define TOK_OCTET      27
#define TOK_STAR       28
#define TOK_DOC        29
#define TOK_IMPORT     30
#endif

typedef struct token_s {
    tok_kind_t kind;
    const char *ptr;
    uint32_t len;
    uint32_t line;
    uint32_t col;
} token_t;

typedef struct lexer_s lexer_t;

lexer_t *lexer_create(const char *src);
void lexer_destroy(lexer_t *lx);
token_t lexer_next(lexer_t *lx);

#endif /* NCOM_TOOLS_NIDL_NIDL_LEX_H */
