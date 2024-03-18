#ifndef AST_H
#define AST_H

#include "common.h"
#include "span.h"
#include "tokens.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct decl_t decl_t;
typedef struct expr_t expr_t;
typedef struct stmt_t stmt_t;
typedef struct type_t type_t;
typedef struct param_t param_t;

typedef array_t(stmt_t *) stmts_t;
typedef array_t(expr_t *) exprs_t;
typedef array_t(param_t *) params_t;

struct decl_t {
    char *id;
    span_t span;
    option_t(type_t) type;
    expr_t *value;
    bool constant;
};

struct stmt_t {
    enum { S_DECL, S_EXPR } type;
    span_t span;

    union {
        decl_t *decl;
        expr_t *expr;
    };
};

struct expr_t {
    enum {
        E_IDENT,
        E_STRING,
        // E_TEMPLATED_STRING,
        E_FN,
        E_INT,
        E_FLOAT,
        E_BINOP,
    } type;
    span_t span;

    union {
        char *ident;
        const char *string;
        // struct {
        //     exprs_t exprs;
        // } templated_string;
        struct {
            params_t params;
            option_t(type_t) ret_type;
            stmts_t stmts;
        } fn;
        i64 int_;
        double float_;
        struct {
            expr_t *lhs, *rhs;
            u8 op; // FIXME: proper binop type
        } binop;
    };
};

struct type_t {
    enum {
        TY_UD,
        TY_PTR,
    } type;
    span_t span;

    union {
        const char *ud;
        struct {
            type_t *inner;
        } ptr;
    };
};

struct param_t {
    char *id;
    span_t span;
    option_t(type_t) type;
    option_t(expr_t) expr;
};

/* -------------------- DEBUGING SHIT -------------------- */

static inline void dump_stmt(stmt_t *, u8);
static inline void dump_expr(expr_t *, u8);
static inline void dump_type(type_t *);
static inline void dump_param(param_t *);

static inline void dump_decl(decl_t *decl, u8 indent) {
    if (decl == NULL) return;
    if (indent > 0)
        for (u8 i = 0; i < indent; i++)
            printf("  ");
    printf("%s %s ", decl->id, decl->constant ? "::" : ":=");
    dump_expr(decl->value, 0);
    printf("\n");
}

static inline void dump_stmt(stmt_t *stmt, u8 indent) {
    if (stmt == NULL) return;
    if (indent > 0)
        for (u8 i = 0; i < indent; i++)
            printf("  ");

    switch (stmt->type) {
    case S_DECL:
        dump_decl(stmt->decl, indent);
        break;

    case S_EXPR:
        dump_expr(stmt->expr, indent);
        break;
    }
}

static inline void dump_expr(expr_t *expr, u8 indent) {
    if (expr == NULL) return;

    if (indent > 0)
        for (u8 i = 0; i < indent; i++)
            printf("  ");

    switch (expr->type) {
    case E_IDENT:
        printf("%s", expr->ident);
        break;

    case E_STRING:
        printf("\"%s\"", expr->string);
        break;

    case E_FN: {
        printf("(");
        for (usz i = 0; i < expr->fn.params.count; i++) {
            dump_param(expr->fn.params.items[i]);
            if (i + 1 < expr->fn.params.count) printf(", ");
        }
        printf(")");
        if (expr->fn.ret_type != NULL) {
            printf(" -> ");
            dump_type(expr->fn.ret_type);
        }
        printf(" {\n");
        for (usz i = 0; i < expr->fn.stmts.count; i++) {
            dump_stmt(expr->fn.stmts.items[i], indent + 1);
            if (i + 1 < expr->fn.stmts.count) printf(";\n");
        }
        printf("\n}");
    } break;

    case E_INT:
        printf("%lld", expr->int_);
        break;

    case E_FLOAT:
        printf("%f", expr->float_);
        break;

    case E_BINOP:
        printf("(");
        dump_expr(expr->binop.lhs, 0);
        printf(" %s ", tt_name(expr->binop.op));
        dump_expr(expr->binop.rhs, 0);
        printf(")");
        break;
    }
}

static inline void dump_type(type_t *type) {
    if (type == NULL) return;

    switch (type->type) {
    case TY_PTR:
        printf("*");
        dump_type(type->ptr.inner);
        break;

    case TY_UD:
        printf("%s", type->ud);
        break;
    }
}

static inline void dump_param(param_t *param) {
    printf("%s", param->id);
    if (param->type != NULL) {
        printf(": ");
        dump_type(param->type);
    }
    if (param->expr != NULL) {
        printf(" %s= ", param->type == NULL ? ":" : "");
        dump_expr(param->expr, 0);
    }
}

#endif // !AST_H