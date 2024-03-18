#ifndef LEXER_H
#define LEXER_H

#include "common.h"
#include "tokens.h"

typedef struct {
    char *source, *filename;
    usz length, pos;
    bool in_string;
} lexer_t;

void l_init(lexer_t *, char *, char *);
void l_free(lexer_t *);
void l_next(lexer_t *, token_t *);

#endif // !LEXER_H
