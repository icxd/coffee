#ifndef SPAN_H
#define SPAN_H

#include "common.h"
#include <string.h>

typedef struct {
    usz start, end;
} span_t;

typedef struct {
    usz line, column;
} source_loc_t;

static inline source_loc_t span_to_source_loc(span_t span, const char *source) {
    usz line = 1, column = 1;
    for (usz i = 0; i < span.start; i++) {
        if (source[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }
    return (source_loc_t){line, column};
}

#endif // !SPAN_H
