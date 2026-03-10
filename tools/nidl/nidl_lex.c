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
 * @file nidl_lex.c
 * @brief Nidl lex.
 */

#include "nidl_lex.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct lexer_s {
    const char *s;
    size_t i;
    uint32_t line;
    uint32_t col;
};

static int is_ident_start(int c) { return isalpha(c) || c == '_'; }
static int is_ident(int c) { return isalnum(c) || c == '_'; }

static token_t make(tok_kind_t k, const char *p, uint32_t n, uint32_t line, uint32_t col) {
    token_t t; t.kind = k; t.ptr = p; t.len = n; t.line=line; t.col=col; return t;
}

lexer_t *lexer_create(const char *src)
{
    lexer_t *lx = (lexer_t *)calloc(1, sizeof(*lx));
    if (!lx) return NULL;
    lx->s = src ? src : "";
    lx->i = 0;
    lx->line = 1;
    lx->col = 1;
    return lx;
}

void lexer_destroy(lexer_t *lx) { free(lx); }

static void adv(lexer_t *lx, char c)
{
    if (c == '\n') { lx->line++; lx->col = 1; }
    else { lx->col++; }
    lx->i++;
}

static void skip_ws_and_comments(lexer_t *lx)
{
    for (;;) {
        char c = lx->s[lx->i];

        while (c && (c==' ' || c=='\t' || c=='\r' || c=='\n')) {
            adv(lx, c);
            c = lx->s[lx->i];
        }

        if (c=='/' && lx->s[lx->i+1]=='/' && lx->s[lx->i+2]=='/') {
            break; /* doc comment */
        }

        if (c=='/' && lx->s[lx->i+1]=='/') {
            adv(lx, '/'); adv(lx, '/');
            while (lx->s[lx->i] && lx->s[lx->i] != '\n') adv(lx, lx->s[lx->i]);
            continue;
        }

        if (c=='/' && lx->s[lx->i+1]=='*' && lx->s[lx->i+2]=='*') {
            break; /* doc comment */
        }

        if (c=='/' && lx->s[lx->i+1]=='*') {
            adv(lx, '/'); adv(lx, '*');
            while (lx->s[lx->i] && !(lx->s[lx->i]=='*' && lx->s[lx->i+1]=='/')) adv(lx, lx->s[lx->i]);
            if (lx->s[lx->i]) { adv(lx, '*'); adv(lx, '/'); }
            continue;
        }

        break;
    }
}

static tok_kind_t keyword(const char *p, uint32_t n)
{
    if (n==6 && memcmp(p,"module",6)==0) return TOK_MODULE;
    if (n==9 && memcmp(p,"interface",9)==0) return TOK_INTERFACE;
    if (n==6 && memcmp(p,"struct",6)==0) return TOK_STRUCT;
    if (n==7 && memcmp(p,"typedef",7)==0) return TOK_TYPEDEF;
    if (n==7 && memcmp(p,"coclass",7)==0) return TOK_COCLASS;
    if (n==2 && memcmp(p,"in",2)==0) return TOK_IN;
    if (n==3 && memcmp(p,"out",3)==0) return TOK_OUT;
    if (n==5 && memcmp(p,"inout",5)==0) return TOK_INOUT;
    if (n==5 && memcmp(p,"const",5)==0) return TOK_CONST;
    if (n==8 && memcmp(p,"unsigned",8)==0) return TOK_UNSIGNED;
    if (n==6 && memcmp(p,"signed",6)==0) return TOK_SIGNED;
    if (n==4 && memcmp(p,"long",4)==0) return TOK_LONG;
    if (n==5 && memcmp(p,"short",5)==0) return TOK_SHORT;
    if (n==3 && memcmp(p,"int",3)==0) return TOK_INT_KW;
    if (n==4 && memcmp(p,"char",4)==0) return TOK_CHAR_KW;
    if (n==5 && memcmp(p,"octet",5)==0) return TOK_OCTET;
    if (n==6 && memcmp(p,"import",6)==0) return TOK_IMPORT;
    return TOK_IDENT;
}

token_t lexer_next(lexer_t *lx)
{
    skip_ws_and_comments(lx);

    const char *s = lx->s;
    size_t i = lx->i;
    uint32_t line = lx->line;
    uint32_t col = lx->col;

    char c = s[i];
    if (!c) return make(TOK_EOF, s+i, 0, line, col);


    /* doc comment: ///... or JAVADOC COMMENT  */
    if (c == '/' && s[i+1] == '/' && s[i+2] == '/') {
        adv(lx, '/'); adv(lx, '/'); adv(lx, '/');
        size_t start = lx->i;
        while (s[lx->i] && s[lx->i] != '\n') adv(lx, s[lx->i]);
        size_t end = lx->i;
        return make(TOK_DOC, s+start, (uint32_t)(end-start), line, col);
    }
    if (c == '/' && s[i+1] == '*' && s[i+2] == '*') {
        adv(lx, '/'); adv(lx, '*'); adv(lx, '*');
        size_t start = lx->i;
        while (s[lx->i] && !(s[lx->i]=='*' && s[lx->i+1]=='/')) adv(lx, s[lx->i]);
        size_t end = lx->i;
        if (s[lx->i]) { adv(lx, '*'); adv(lx, '/'); }
        return make(TOK_DOC, s+start, (uint32_t)(end-start), line, col);
    }


    switch (c) {
        case '{': adv(lx,c); return make(TOK_LBRACE, s+i, 1, line, col);
        case '}': adv(lx,c); return make(TOK_RBRACE, s+i, 1, line, col);
        case '(': adv(lx,c); return make(TOK_LPAREN, s+i, 1, line, col);
        case ')': adv(lx,c); return make(TOK_RPAREN, s+i, 1, line, col);
        case '[': adv(lx,c); return make(TOK_LBRACKET, s+i, 1, line, col);
        case ']': adv(lx,c); return make(TOK_RBRACKET, s+i, 1, line, col);
        case ':': adv(lx,c); return make(TOK_COLON, s+i, 1, line, col);
        case ';': adv(lx,c); return make(TOK_SEMI, s+i, 1, line, col);
        case ',': adv(lx,c); return make(TOK_COMMA, s+i, 1, line, col);
        case '*': adv(lx,c); return make(TOK_STAR, s+i, 1, line, col);
        default: break;
    }

    if (c == '"') {
        adv(lx, '"');
        size_t start = lx->i;
        while (s[lx->i] && s[lx->i] != '"') {
            if (s[lx->i] == '\\' && s[lx->i+1]) { adv(lx, s[lx->i]); adv(lx, s[lx->i]); }
            else adv(lx, s[lx->i]);
        }
        size_t end = lx->i;
        if (s[lx->i] == '"') adv(lx, '"');
        return make(TOK_STRING, s+start, (uint32_t)(end-start), line, col);
    }

    if (is_ident_start((unsigned char)c)) {
        size_t start = lx->i;
        adv(lx, s[lx->i]);
        while (is_ident((unsigned char)s[lx->i])) adv(lx, s[lx->i]);
        size_t end = lx->i;
        tok_kind_t k = keyword(s+start, (uint32_t)(end-start));
        return make(k, s+start, (uint32_t)(end-start), line, col);
    }

    adv(lx, c);
    return make(TOK_IDENT, s+i, 1, line, col);
}
