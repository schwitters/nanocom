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
