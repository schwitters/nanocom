/*!re2c
re2c:define:YYCTYPE = "unsigned char";
re2c:yyfill:enable = 0;
re2c:api:style = free-form;
*/

#include "nidl_lex.h"
#include <string.h>

struct lexer_s {
    const char *s;
    const char *cur;
    uint32_t line;
    uint32_t col;
};

static void adv_pos(lexer_t *lx, const unsigned char *from, const unsigned char *to)
{
    for (const unsigned char *p = from; p < to; p++) {
        if (*p == '\n') { lx->line++; lx->col = 1; }
        else { lx->col++; }
    }
}

static token_t tok(tok_kind_t k, const char *p, uint32_t n, uint32_t line, uint32_t col)
{
    token_t t;
    t.kind = k;
    t.ptr = p;
    t.len = n;
    t.line = line;
    t.col = col;
    return t;
}

static tok_kind_t kw(const char *p, uint32_t n)
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
    return TOK_IDENT;
}

token_t lexer_next_re2c(lexer_t *lx)
{
restart:
    {
        const unsigned char *YYCURSOR = (const unsigned char *)lx->cur;
        const unsigned char *YYMARKER = YYCURSOR;
        const unsigned char *ts = YYCURSOR;
        uint32_t line = lx->line;
        uint32_t col  = lx->col;

        /*!re2c
        WS      = [ \t\r\n]+;
        ID0     = [A-Za-z_];
        ID      = ID0 [A-Za-z0-9_]*;
        DQ      = "\"";
        ESC     = "\\" [^\n];
        STRCHR  = [^"\\\n] | ESC;
        STR     = DQ STRCHR* DQ;

        DOC_LINE   = "///" [^\n]*;
DOC_BLOCK  = "/**" ([^*] | "*" [^/])* "*/";
SLASHSLASH = "//" [^\n]*;
SLASHSTAR  = "/*" ([^*] | "*" [^/])* "*/";

        DOC_LINE    {
              adv_pos(lx, ts, YYCURSOR);
              lx->cur = (const char*)YYCURSOR;
              const char *p = (const char*)ts;
              uint32_t n = (uint32_t)(YYCURSOR-ts);
              return tok(TOK_DOC, p+3, n-3, line, col);
            }
        DOC_BLOCK   {
              adv_pos(lx, ts, YYCURSOR);
              lx->cur = (const char*)YYCURSOR;
              const char *p = (const char*)ts;
              uint32_t n = (uint32_t)(YYCURSOR-ts);
              if (n >= 5) return tok(TOK_DOC, p+3, n-5, line, col);
              return tok(TOK_DOC, p, n, line, col);
            }
        WS          { adv_pos(lx, ts, YYCURSOR); lx->cur = (const char*)YYCURSOR; goto restart; }
        SLASHSLASH  { adv_pos(lx, ts, YYCURSOR); lx->cur = (const char*)YYCURSOR; goto restart; }
        SLASHSTAR   { adv_pos(lx, ts, YYCURSOR); lx->cur = (const char*)YYCURSOR; goto restart; }

        "{"         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_LBRACE, "{", 1, line, col); }
        "}"         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_RBRACE, "}", 1, line, col); }
        "("         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_LPAREN, "(", 1, line, col); }
        ")"         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_RPAREN, ")", 1, line, col); }
        "["         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_LBRACKET, "[", 1, line, col); }
        "]"         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_RBRACKET, "]", 1, line, col); }
        ":"         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_COLON, ":", 1, line, col); }
        ";"         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_SEMI, ";", 1, line, col); }
        ","         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_COMMA, ",", 1, line, col); }
        "*"         { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_STAR, "*", 1, line, col); }
        ID          {
                        adv_pos(lx, ts, YYCURSOR);
                        lx->cur=(const char*)YYCURSOR;
                        uint32_t n = (uint32_t)(YYCURSOR-ts);
                        tok_kind_t k = kw((const char*)ts, n);
                        return tok(k, (const char*)ts, n, line, col);
                    }
        STR         {
                        adv_pos(lx, ts, YYCURSOR);
                        lx->cur=(const char*)YYCURSOR;
                        const char *p = (const char*)ts;
                        uint32_t n = (uint32_t)(YYCURSOR-ts);
                        return tok(TOK_STRING, p+1, n-2, line, col);
                    }

        "\000"      { lx->cur=(const char*)YYCURSOR; return tok(TOK_EOF, (const char*)YYCURSOR, 0, line, col); }

        *           { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_IDENT, (const char*)ts, 1, line, col); }
        */
    }
}
