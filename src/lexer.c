#include "include/lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void l_init(lexer_t *l, char *source, char *filename) {
    l->source = source;
    l->filename = filename;
    l->length = strlen(source);
    l->pos = 0;
}

void l_free(lexer_t *l) {
    free(l->source);
    free(l);
}

void l_next(lexer_t *l, token_t *token) {
    if (l->pos >= l->length) {
        token->type = T_EOF;
        token->span = (span_t){l->pos, l->pos + 1};
        return;
    }

    while (isspace(l->source[l->pos]))
        l->pos++;

    char ch = l->source[l->pos];
    switch (ch) {

#define SINGLE(ch, tok)                                                        \
    case (ch): {                                                               \
        token->type = l->in_string ? T_STRING_MIDDLE : (tok);                  \
        token->span = (span_t){l->pos, l->pos + 1};                            \
        l->pos++;                                                              \
    } break
#define DOUBLE(ch, ch2, tok, tok2)                                             \
    case (ch): {                                                               \
        if (l->pos + 1 < l->length && l->source[l->pos + 1] == (ch2)) {        \
            token->type = l->in_string ? T_STRING_MIDDLE : tok2;               \
            token->span = (span_t){l->pos, l->pos + 2};                        \
            l->pos += 2;                                                       \
        } else {                                                               \
            token->type = l->in_string ? T_STRING_MIDDLE : (tok);              \
            token->span = (span_t){l->pos, l->pos + 1};                        \
            l->pos++;                                                          \
        }                                                                      \
    } break

        DOUBLE('+', '=', T_PLUS, T_PLUS_EQUALS);

    case '-': {
        if (l->pos + 1 < l->length && l->source[l->pos + 1] == '>') {
            token->type = l->in_string ? T_STRING_MIDDLE : T_ARROW;
            token->span = (span_t){l->pos, l->pos + 2};
            l->pos += 2;
        } else if (l->pos + 1 < l->length && l->source[l->pos + 1] == '=') {
            token->type = l->in_string ? T_STRING_MIDDLE : T_MINUS_EQUALS;
            token->span = (span_t){l->pos, l->pos + 2};
            l->pos += 2;
        } else {
            token->type = l->in_string ? T_STRING_MIDDLE : T_MINUS;
            token->span = (span_t){l->pos, l->pos + 1};
            l->pos++;
        }
    } break;

        DOUBLE('*', '=', T_ASTERISK, T_ASTERISK_EQUALS);
        DOUBLE('/', '=', T_SLASH, T_SLASH_EQUALS);
        DOUBLE('%', '=', T_PERCENT, T_PERCENT_EQUALS);
        DOUBLE('=', '=', T_EQUALS, T_EQUALS_EQUALS);
        DOUBLE('!', '=', T_BANG, T_BANG_EQUALS);
        DOUBLE('<', '=', T_LESS_THAN, T_LESS_THAN_EQUALS);
        DOUBLE('>', '=', T_GREATER_THAN, T_GREATER_THAN_EQUALS);
        DOUBLE('?', '?', T_QUESTION, T_QUESTION_QUESTION);

        SINGLE('(', T_OPEN_PAREN);
        SINGLE(')', T_CLOSE_PAREN);
        SINGLE('{', T_OPEN_BRACE);
        SINGLE('}', T_CLOSE_BRACE);
        SINGLE(';', T_SEMICOLON);

    case ':': {
        if (l->pos + 1 < l->length && l->source[l->pos + 1] == ':') {
            token->type = l->in_string ? T_STRING_MIDDLE : T_COLON_COLON;
            token->span = (span_t){l->pos, l->pos + 2};
            l->pos += 2;
        } else if (l->pos + 1 < l->length && l->source[l->pos + 1] == '=') {
            token->type = l->in_string ? T_STRING_MIDDLE : T_COLON_EQUALS;
            token->span = (span_t){l->pos, l->pos + 2};
            l->pos += 2;
        } else {
            token->type = l->in_string ? T_STRING_MIDDLE : T_COLON;
            token->span = (span_t){l->pos, l->pos + 1};
            l->pos++;
        }
    } break;

        SINGLE(',', T_COMMA);

    case '"': {
        // "abc \(1 + 2 * 3) def"
        // if (!l->in_string) {
        //     l->in_string = true;
        //     token->type = T_STRING_START;
        //     token->span = (span_t){l->pos, l->pos + 1};
        //     l->pos++;
        // } else {
        //     l->in_string = false;
        //     token->type = T_STRING_END;
        //     token->span = (span_t){l->pos, l->pos + 1};
        //     l->pos++;
        // }

        token->type = T_STRING;
        l->pos++;
        usz start = l->pos;
        while (l->pos < l->length && l->source[l->pos] != '"')
            l->pos++;
        token->span = (span_t){start, l->pos};
        token->string_value = malloc(l->pos - start);
        strncpy(token->string_value, l->source + start, l->pos - start);
        token->string_value[l->pos - start] = '\0';
        l->pos++;
        break;
    } break;

    default: {
        if (isalpha(ch)) {
            token->type = l->in_string ? T_STRING_MIDDLE : T_IDENT;
            usz start = l->pos;
            while (l->pos < l->length && isalnum(l->source[l->pos]))
                l->pos++;
            token->span = (span_t){start, l->pos};
            token->string_value = malloc(l->pos - start);
            strncpy(token->string_value, l->source + start, l->pos - start);
            token->string_value[l->pos - start] = '\0';
            break;
        }

        if (isdigit(ch)) {
            token->type = l->in_string ? T_STRING_MIDDLE : T_INT;
            usz start = l->pos;

            enum { I_DEC, I_HEX, I_BIN } int_type = I_DEC;

            if (ch == '0' && l->pos + 1 < l->length &&
                l->source[l->pos + 1] == 'x') {
                int_type = I_HEX;
                l->pos += 2;
                while (l->pos < l->length && isxdigit(l->source[l->pos]))
                    l->pos++;
            } else if (ch == '0' && l->pos + 1 < l->length &&
                       l->source[l->pos + 1] == 'b') {
                int_type = I_BIN;
                l->pos += 2;
                l->pos += 2;
                while (l->pos < l->length &&
                       (l->source[l->pos] == '0' || l->source[l->pos] == '1'))
                    l->pos++;
            } else {
                while (l->pos < l->length && isdigit(l->source[l->pos]))
                    l->pos++;

                if (l->source[l->pos] == '.') {
                    token->type = T_FLOAT;
                    l->pos++;
                    while (l->pos < l->length && isdigit(l->source[l->pos]))
                        l->pos++;
                }
            }

            token->span = (span_t){start, l->pos};
            char *buffer = malloc(l->pos - start);
            strncpy(buffer, l->source + start, l->pos - start);
            buffer[l->pos - start] = '\0';
            if (token->type == T_INT) {
                switch (int_type) {
                case I_DEC:
                    token->int_value = atoll(buffer);
                    break;
                case I_HEX:
                    token->int_value = strtoll(buffer, NULL, 16);
                    break;
                case I_BIN:
                    token->int_value = strtoll(buffer, NULL, 2);
                    break;
                }
            } else token->float_value = atof(buffer);
            break;
        }

        token->type = l->in_string ? T_STRING_MIDDLE : T_ERROR;
        token->span = (span_t){l->pos, l->pos + 1};
        token->string_value = realloc(token->string_value, 1);
        token->string_value[0] = ch;
        token->string_value[1] = '\0';
        l->pos++;
    } break;
    }

#undef SINGLE
#undef DOUBLE
}