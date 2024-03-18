#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "common.h"
#include "error.h"
#include "lexer.h"
#include <stdbool.h>

typedef struct {
    lexer_t *lexer;
    token_t token;
    errors_t errors;
} parser_t;

void p_init(parser_t *, lexer_t *);
void p_free(parser_t *);

void p_advance(parser_t *);
bool p_expect(parser_t *, u8);
void p_error(parser_t *, const char *, ...);

decl_t *p_parse_decl(parser_t *);
stmt_t *p_parse_stmt(parser_t *);

expr_t *p_parse_expr(parser_t *);
expr_t *p_parse_precedence(parser_t *, expr_t *, u8);
expr_t *p_parse_primary(parser_t *);

type_t *p_parse_type(parser_t *);
param_t *p_parse_param(parser_t *);

#endif // !PARSER_H