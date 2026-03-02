/*!re2c
re2c:define:YYCTYPE = "unsigned char";
re2c:yyfill:enable = 0;
re2c:api:style = free-form;
*/

#include "nidl_lex.h"   /* token_t, tok_kind_t, and TOK_* token codes */
#include <string.h>     /* memcmp */

/**
 * @brief Lexer state for the re2c-generated scanner.
 *
 * This struct is mirrored in the wrapper that allocates/initializes the lexer.
 *
 * - s:   start of source buffer (NUL-terminated)
 * - cur: current scan pointer (YYCURSOR is initialized from this)
 * - line/col: 1-based position tracking for diagnostics
 */
struct lexer_s {
    const char *s;
    const char *cur;
    uint32_t line;
    uint32_t col;
};

/**
 * @brief Advance line/column counters for a consumed range.
 *
 * re2c tells us the match range via pointers [from,to).
 * We update (line,col) so error messages can report accurate positions.
 *
 * NOTE:
 * - column resets to 1 after '\n'
 * - this counts bytes, not Unicode codepoints (fine for IDL source)
 */
static void adv_pos(lexer_t *lx, const unsigned char *from, const unsigned char *to)
{
    for (const unsigned char *p = from; p < to; p++) {
        if (*p == '\n') { lx->line++; lx->col = 1; }
        else { lx->col++; }
    }
}

/**
 * @brief Construct a token_t value.
 *
 * @param k    token kind (TOK_*)
 * @param p    pointer into the input buffer (borrowed)
 * @param n    token length in bytes
 * @param line starting line (captured before scanning)
 * @param col  starting column (captured before scanning)
 */
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

/**
 * @brief Keyword classification for identifiers.
 *
 * IDL keywords are recognized by exact length+memcmp, returning specific TOK_* values.
 * Non-keywords become TOK_IDENT.
 *
 * IMPORTANT:
 * - The lexer recognizes keywords *only* in the ID rule (identifier tokens).
 * - Punctuation and literals are handled by separate rules.
 */
static tok_kind_t kw(const char *p, uint32_t n)
{
    if (n==6 && memcmp(p,"module",6)==0) return TOK_MODULE;
    if (n==9 && memcmp(p,"interface",9)==0) return TOK_INTERFACE;
    if (n==6 && memcmp(p,"struct",6)==0) return TOK_STRUCT;
    if (n==7 && memcmp(p,"typedef",7)==0) return TOK_TYPEDEF;
    if (n==7 && memcmp(p,"coclass",7)==0) return TOK_COCLASS;

    /* parameter directions */
    if (n==2 && memcmp(p,"in",2)==0) return TOK_IN;
    if (n==3 && memcmp(p,"out",3)==0) return TOK_OUT;
    if (n==5 && memcmp(p,"inout",5)==0) return TOK_INOUT;

    /* type_spec keywords */
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

/**
 * @brief Return the next token from the input.
 *
 * This is the core re2c-driven lexer. It:
 * - scans from lx->cur
 * - returns one token
 * - updates lx->cur and (line,col)
 *
 * Whitespace and non-doc comments are skipped by looping to `restart:`.
 */
token_t lexer_next_re2c(lexer_t *lx)
{
restart:
    {
        /* re2c cursor variables:
         * - YYCURSOR points at current input position
         * - YYMARKER is used by re2c internally for backtracking
         * - ts ("token start") is our own pointer to match start for the current rule
         */
        const unsigned char *YYCURSOR = (const unsigned char *)lx->cur;
        const unsigned char *YYMARKER = YYCURSOR;
        const unsigned char *ts = YYCURSOR;

        /* Capture the starting position for this token before consuming. */
        uint32_t line = lx->line;
        uint32_t col  = lx->col;

        /*!re2c
        /* ====== Basic tokens ====== */

        /* Whitespace: skipped (does not produce a token). */
        WS      = [ \t\r\n]+;

        /* Identifier:
         * - first char: letter or underscore
         * - remaining: letters, digits, underscore
         */
        ID0     = [A-Za-z_];
        ID      = ID0 [A-Za-z0-9_]*;

        /* String literal token:
         * - DQ: double quote
         * - ESC: backslash followed by any non-newline char (simple escape model)
         * - STRCHR: any non-quote, non-backslash, non-newline char OR ESC
         * - STR: DQ (STRCHR*) DQ
         *
         * The lexer returns TOK_STRING with the quotes stripped (p+1, n-2).
         */
        DQ      = "\"";
        ESC     = "\\" [^\n];
        STRCHR  = [^"\\\n] | ESC;
        STR     = DQ STRCHR* DQ;

        /* ====== Documentation and comments ======
         *
         * DOC_LINE:  /// until newline (returned as TOK_DOC, without leading "///")
         * DOC_BLOCK: java style config block (returned as TOK_DOC, without leading java style doc block start)
         *
         * SLASHSLASH and SLASHSTAR are non-doc comments and are skipped.
         *
         * NOTE: DOC_BLOCK pattern is simplistic and does not handle nested comments
         * (same as C). It’s sufficient for IDL doc blocks.
         */
        DOC_LINE   = "///" [^\n]*;
        DOC_BLOCK  = "/**" ([^*] | "*" [^/])* "*/";
        SLASHSLASH = "//" [^\n]*;
        SLASHSTAR  = "/*" ([^*] | "*" [^/])* "*/";

        /* ====== Doc emission (returns TOK_DOC) ====== */
        DOC_LINE    {
              adv_pos(lx, ts, YYCURSOR);
              lx->cur = (const char*)YYCURSOR;

              /* Strip the '///' prefix. */
              const char *p = (const char*)ts;
              uint32_t n = (uint32_t)(YYCURSOR-ts);
              return tok(TOK_DOC, p+3, n-3, line, col);
            }

        DOC_BLOCK   {
              adv_pos(lx, ts, YYCURSOR);
              lx->cur = (const char*)YYCURSOR;

              /* Strip the strip java document chars */
              const char *p = (const char*)ts;
              uint32_t n = (uint32_t)(YYCURSOR-ts);
              if (n >= 5) return tok(TOK_DOC, p+3, n-5, line, col);
              return tok(TOK_DOC, p, n, line, col);
            }

        /* ====== Skipped tokens (whitespace and non-doc comments) ====== */
        WS          { adv_pos(lx, ts, YYCURSOR); lx->cur = (const char*)YYCURSOR; goto restart; }
        SLASHSLASH  { adv_pos(lx, ts, YYCURSOR); lx->cur = (const char*)YYCURSOR; goto restart; }
        SLASHSTAR   { adv_pos(lx, ts, YYCURSOR); lx->cur = (const char*)YYCURSOR; goto restart; }

        /* ====== Punctuation tokens ======
         * These match single-character punctuation used by the grammar.
         */
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

        /* ====== Identifier / keyword ======
         * On match, call kw() to decide whether it's a keyword or plain identifier.
         */
        ID          {
                        adv_pos(lx, ts, YYCURSOR);
                        lx->cur=(const char*)YYCURSOR;
                        uint32_t n = (uint32_t)(YYCURSOR-ts);
                        tok_kind_t k = kw((const char*)ts, n);
                        return tok(k, (const char*)ts, n, line, col);
                    }

        /* ====== String literal ======
         * Return the content without quotes.
         * ESC handling is "light": we don't unescape here, we just allow escaped chars.
         */
        STR         {
                        adv_pos(lx, ts, YYCURSOR);
                        lx->cur=(const char*)YYCURSOR;
                        const char *p = (const char*)ts;
                        uint32_t n = (uint32_t)(YYCURSOR-ts);
                        return tok(TOK_STRING, p+1, n-2, line, col);
                    }

        /* ====== End of input ====== */
        "\000"      { lx->cur=(const char*)YYCURSOR; return tok(TOK_EOF, (const char*)YYCURSOR, 0, line, col); }

        /* ====== Fallback ======
         * If nothing matched, consume one byte and return it as TOK_IDENT.
         * This keeps the lexer progressing and lets the parser report a syntax error.
         *
         * NOTE: For stricter behavior you could return a dedicated TOK_ERROR token.
         */
        *           { adv_pos(lx, ts, YYCURSOR); lx->cur=(const char*)YYCURSOR; return tok(TOK_IDENT, (const char*)ts, 1, line, col); }
        */
    }
}