#include "include/parser.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void p_init(parser_t *p, lexer_t *l) {
    p->lexer = l;
    p_advance(p);
    p->errors = (errors_t){0};
}

void p_free(parser_t *p) { free(p); }

void p_advance(parser_t *p) { l_next(p->lexer, &p->token); }

bool p_expect(parser_t *p, u8 type) {
    if (p->token.type == T_ERROR) {
        p_error(p, "unexpected character `%s`", p->token.string_value);
        exit(1);
    }
    if (p->token.type != type) return false;
    p_advance(p);
    return true;
}

static void vasprintf(char **strp, const char *fmt, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);
    size_t size = vsnprintf(NULL, 0, fmt, ap2) + 1;
    va_end(ap2);
    *strp = malloc(size);
    if (*strp == NULL) {
        perror("malloc() failed");
        exit(1);
    }
    vsnprintf(*strp, size, fmt, ap);
}

void p_error(parser_t *p, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);

    source_loc_t loc = span_to_source_loc(p->token.span, p->lexer->source);

    char *message;
    vasprintf(&message, msg, ap);

    error_t err = (error_t){
        .span = p->token.span,
        .source_loc = loc,
        .msg = message,
    };
    da_append(&p->errors, err);

    va_end(ap);
}

#define E_EXPECT(parser, expected)                                             \
    p_error((parser), "expected `%s`, but got `%s` instead",                   \
            tt_name(expected), tt_name(parser->token.type))

decl_t *p_parse_decl(parser_t *p) {
    decl_t *decl = malloc(sizeof(decl_t));

    decl->id = p->token.string_value;
    decl->span = p->token.span;
    if (!p_expect(p, T_IDENT)) {
        E_EXPECT(p, T_IDENT);
        return NULL;
    }

    decl->constant = true;
    if (!p_expect(p, T_COLON_COLON)) {
        E_EXPECT(p, T_COLON_COLON);
        return NULL;
    }

    expr_t *value = p_parse_expr(p);
    if (value == NULL) return NULL;
    decl->value = value;

    return decl;
}

stmt_t *p_parse_stmt(parser_t *p) {
    stmt_t *stmt = malloc(sizeof(stmt_t));

    // in the case where p_parse_decl fails, we need to rollback the parser
    // to before we attempted to parse it to make sure that all the state
    // is as it should be before we return NULL
    usz error_count = p->errors.count;
    usz token_index = p->lexer->pos;
    span_t token_span = p->token.span;

    decl_t *decl = p_parse_decl(p);
    if (decl != NULL) {
        stmt->type = S_DECL;
        stmt->decl = decl;
    } else {
        usz error_count2 = p->errors.count;
        usz token_index2 = p->lexer->pos;
        span_t token_span2 = p->token.span;

        while (p->errors.count > error_count) {
            error_t error = da_pop(&p->errors);
            free(error.msg);
        }
        p->lexer->pos = token_index;
        p->token.span = token_span;

        expr_t *expr = p_parse_expr(p);
        if (expr == NULL) {
            while (p->errors.count > error_count2) {
                error_t error = da_pop(&p->errors);
                free(error.msg);
            }
            p->lexer->pos = token_index2;
            p->token.span = token_span2;

            p_error(p, "expected a statement, but got `%s` instead",
                    tt_name(p->token.type));
            return NULL;
        }

        stmt->type = S_EXPR;
        stmt->expr = expr;
    }

    return stmt;
}

expr_t *p_parse_expr(parser_t *p) {
    expr_t *lhs = p_parse_primary(p);
    if (lhs == NULL) return NULL;
    return p_parse_precedence(p, lhs, 0);
}

expr_t *p_parse_precedence(parser_t *p, expr_t *lhs, u8 min_precedence) {
    lhs->span.end = p->token.span.start;
    token_t lookahead = p->token;

    while (tt_is_binop(lookahead.type) &&
           tt_precedence(lookahead.type) >= min_precedence) {
        u8 op = lookahead.type;
        p_advance(p);
        expr_t *rhs = p_parse_primary(p);
        if (rhs == NULL) return NULL;
        lookahead = p->token;

        while (tt_is_binop(lookahead.type) &&
               tt_precedence(lookahead.type) > tt_precedence(op)) {
            rhs = p_parse_precedence(p, rhs, tt_precedence(op) + 1);
            lookahead = p->token;
        }

        expr_t *expr = malloc(sizeof(expr_t));
        expr->type = E_BINOP;
        expr->binop.lhs = lhs;
        expr->binop.rhs = rhs;
        expr->binop.op = op;
        lhs = expr;
        lhs->span.end = rhs->span.end;
    }

    return lhs;
}

expr_t *p_parse_primary(parser_t *p) {
    expr_t *expression = malloc(sizeof(expr_t));
    expression->span = p->token.span;

    switch (p->token.type) {

    case T_IDENT: {
        expression->type = E_IDENT;
        expression->ident = p->token.string_value;
        p_advance(p);
    } break;

    case T_STRING: {
        expression->type = E_STRING;
        expression->string = p->token.string_value;
        p_advance(p);
    } break;

    case T_INT: {
        expression->type = E_INT;
        expression->int_ = p->token.int_value;
        p_advance(p);
    } break;

    case T_FLOAT: {
        expression->type = E_FLOAT;
        expression->float_ = p->token.float_value;
        p_advance(p);
    } break;

    case T_OPEN_PAREN: {
        expression->type = E_FN;

        if (!p_expect(p, T_OPEN_PAREN)) {
            E_EXPECT(p, T_OPEN_PAREN);
            return NULL;
        }

        params_t params = {0};
        for (;;) {
            if (p->token.type == T_CLOSE_PAREN) break;

            param_t *param = p_parse_param(p);
            if (param == NULL) return NULL;
            da_append(&params, param);

            if (p->token.type != T_COMMA) break;
            p_expect(p, T_COMMA);
        }
        expression->fn.params = params;

        if (!p_expect(p, T_CLOSE_PAREN)) {
            E_EXPECT(p, T_CLOSE_PAREN);
            return NULL;
        }

        option_t(type_t) return_type = NULL;
        if (p_expect(p, T_ARROW)) {
            type_t *type = p_parse_type(p);
            if (type == NULL) return NULL;
            return_type = type;
        }
        expression->fn.ret_type = return_type;

        if (!p_expect(p, T_OPEN_BRACE)) {
            E_EXPECT(p, T_OPEN_BRACE);
            return NULL;
        }

        stmts_t stmts = {0};
        for (;;) {
            if (p->token.type == T_CLOSE_BRACE) break;
            stmt_t *stmt = p_parse_stmt(p);
            if (stmt == NULL) return NULL;
            da_append(&stmts, stmt);

            if (p->token.type != T_SEMICOLON) break;
            p_expect(p, T_SEMICOLON);
        }
        expression->fn.stmts = stmts;

        if (!p_expect(p, T_CLOSE_BRACE)) {
            E_EXPECT(p, T_CLOSE_BRACE);
            return NULL;
        }
    } break;

    default: {
        p_error(p, "expected an expression, but got `%s` instead",
                tt_name(p->token.type));
        return NULL;
    }
    }

    return expression;
}

type_t *p_parse_type(parser_t *p) {
    type_t *type = malloc(sizeof(type_t));
    type->span = p->token.span;

    switch (p->token.type) {
    case T_IDENT: {
        type->type = TY_UD;
        type->ud = p->token.string_value;
        p_advance(p);
    } break;

    case T_ASTERISK: {
        type->type = TY_PTR;
        p_advance(p);
        type_t *inner = p_parse_type(p);
        if (inner == NULL) return NULL;
        type->span.end = inner->span.end;
        type->ptr.inner = inner;
    } break;

    default: {
        p_error(p, "expected a type, but got `%s` instead",
                tt_name(p->token.type));
        return NULL;
    }
    }

    return type;
}

param_t *p_parse_param(parser_t *p) {
    param_t *param = malloc(sizeof(param_t));

    char *id = p->token.string_value;
    param->span = p->token.span;
    if (!p_expect(p, T_IDENT)) {
        E_EXPECT(p, T_IDENT);
        return NULL;
    }
    param->id = id;

    option_t(type_t) type = NULL;
    option_t(expr_t) expr = NULL;

    if (p_expect(p, T_COLON_EQUALS)) {
        expr_t *ex = p_parse_expr(p);
        if (ex == NULL) return NULL;
        expr = ex;
    } else if (p_expect(p, T_COLON)) {
        type_t *ty = p_parse_type(p);
        if (ty == NULL) return NULL;
        type = ty;

        if (p_expect(p, T_EQUALS)) {
            expr_t *ex = p_parse_expr(p);
            if (ex == NULL) return NULL;
            expr = ex;
        }
    } else {
        p_error(p, "expected either `:` or `:=` but got %s instead",
                tt_name(p->token.type));
        return NULL;
    }

    param->type = type;
    param->expr = expr;

    return param;
}