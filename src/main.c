#include "include/analyzer.h"
#include "include/ast.h"
#include "include/error.h"
#include "include/lexer.h"
#include "include/log.h"
#include "include/parser.h"
#include "include/tokens.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        log_error("Usage: %s <path> [OPT]", argv[0]);
        return -1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        log_error("fopen() failed: %s", strerror(errno));
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    isz size = ftell(fp);
    rewind(fp);

    char *source = malloc(size);
    if (source == NULL) {
        log_error("malloc() failed: %s", strerror(errno));
        return -1;
    }

    int read = fread(source, 1, size, fp);
    if (read != size) {
        log_error("fread() failed: %s", strerror(errno));
        return -1;
    }

    lexer_t *lexer = malloc(sizeof(lexer_t));
    if (lexer == NULL) {
        log_error("malloc() failed: %s", strerror(errno));
        return -1;
    }

    parser_t *parser = malloc(sizeof(parser_t));
    if (lexer == NULL) {
        log_error("malloc() failed: %s", strerror(errno));
        return -1;
    }

    l_init(lexer, source, argv[1]);
    p_init(parser, lexer);

    // token_t token = {0};
    // do {
    //     l_next(lexer, &token);
    //     printf("%s", tt_name(token.type));
    //     if (token.type == T_IDENT || token.type == T_ERROR)
    //         printf(" (%s)", token.string_value);
    //     if (token.type == T_INT) printf(" (%lld)", token.int_value);
    //     printf("\n");
    //     if (token.type == T_EOF || token.type == T_ERROR) break;
    // } while (1);

    decl_t *decl = p_parse_decl(parser);

    if (parser->errors.count > 0) {
        for (usz i = 0; i < parser->errors.count; i++) {
            error_t error = parser->errors.items[i];
            fprintf(stderr,
                    "\033[0;1m%s:%ld:%ld: \033[31;1merror: \033[0;0m%s\n",
                    argv[1], error.source_loc.line, error.source_loc.column,
                    error.msg);
        }
    }

    dump_decl(decl, 0);

    // analyzer_t *analyzer = malloc(sizeof(analyzer_t));
    // if (analyzer == NULL) {
    //     log_error("malloc() failed: %s", strerror(errno));
    //     return -1;
    // }

    // a_init(analyzer, source, argv[1]);

    // decl = a_eval_decl(analyzer, decl);
    // dump_decl(decl, 0);

    p_free(parser);
    l_free(lexer);
    fclose(fp);

    return 0;
}
