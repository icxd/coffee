#ifndef TOKENS_H
#define TOKENS_H

#include "common.h"
#include "span.h"
#include <assert.h>
#include <stdbool.h>

// https://docs.onyxlang.io/book/operators/precedence.html
#define TOKENS                                                                 \
    T(IDENT, "<ident>")                                                        \
    T(INT, "<int>")                                                            \
    T(FLOAT, "<float>")                                                        \
    T(STRING, "<string>")                                                      \
    T(STRING_START, "<string_start>")                                          \
    T(STRING_END, "<string_end>")                                              \
    T(STRING_MIDDLE, "<string_middle>")                                        \
                                                                               \
    T_BIN(PLUS, "+", true, 5)                                                  \
    T_BIN(PLUS_EQUALS, "+=", true, 1)                                          \
    T_BIN(MINUS, "-", true, 5)                                                 \
    T_BIN(MINUS_EQUALS, "-=", true, 1)                                         \
    T(ARROW, "->")                                                             \
    T_BIN(ASTERISK, "*", true, 6)                                              \
    T_BIN(ASTERISK_EQUALS, "*=", true, 1)                                      \
    T_BIN(SLASH, "/", true, 6)                                                 \
    T_BIN(SLASH_EQUALS, "/=", true, 1)                                         \
    T_BIN(PERCENT, "%", true, 7)                                               \
    T_BIN(PERCENT_EQUALS, "%=", true, 1)                                       \
    T_BIN(EQUALS, "=", true, 1)                                                \
    T_BIN(EQUALS_EQUALS, "==", true, 2)                                        \
    T(BANG, "!")                                                               \
    T_BIN(BANG_EQUALS, "!=", true, 2)                                          \
    T_BIN(LESS_THAN, "<", true, 3)                                             \
    T_BIN(LESS_THAN_EQUALS, "<=", true, 3)                                     \
    T_BIN(GREATER_THAN, ">", true, 3)                                          \
    T_BIN(GREATER_THAN_EQUALS, ">=", true, 3)                                  \
    T(QUESTION, "?")                                                           \
    T_BIN(QUESTION_QUESTION, "??", true, 4)                                    \
                                                                               \
    T(OPEN_PAREN, "(")                                                         \
    T(CLOSE_PAREN, ")")                                                        \
    T(OPEN_BRACE, "{")                                                         \
    T(CLOSE_BRACE, "}")                                                        \
    T(SEMICOLON, ";")                                                          \
    T(COLON, ":")                                                              \
    T(COLON_COLON, "::")                                                       \
    T(COLON_EQUALS, ":=")                                                      \
    T(COMMA, ",")                                                              \
    T(ERROR, "<error>")                                                        \
    T(EOF, "<eof>")

enum {
#define T(id, ...) T_##id,
#define T_BIN(id, ...) T_##id,
    TOKENS
#undef T_BIN
#undef T
};

static inline const char *tt_name(u8 type) {
    const char *names[] = {
#define T(id, name) [T_##id] = name,
#define T_BIN(id, name, ...) [T_##id] = name,
        TOKENS
#undef T_BIN
#undef T
    };
    return names[type];
}

static inline bool tt_is_binop(u8 type) {
    bool is_binop[] = {
#define T(id, ...) [T_##id] = false,
#define T_BIN(id, _a, binop, ...) [T_##id] = binop,
        TOKENS
#undef T_BIN
#undef T
    };
    return is_binop[type];
}

static inline u8 tt_precedence(u8 type) {
    i8 precs[] = {
#define T(id, ...) [T_##id] = -1,
#define T_BIN(id, _a, _b, prec) [T_##id] = prec,
        TOKENS
#undef T_BIN
#undef T
    };
    if (precs[type] < 0) return 128;
    return (u8)precs[type];
}

typedef struct token_t token_t;
typedef array_t(token_t) tokens_t;

struct token_t {
    u8 type;
    span_t span;

    union {
        char *string_value;
        i64 int_value;
        double float_value;
        struct {
            tokens_t tokens;
        } template_string;
    };
};

#endif // !TOKENS_H