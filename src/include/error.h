#ifndef ERROR_H
#define ERROR_H

#include "common.h"
#include "span.h"
#include <stdarg.h>

typedef struct {
    span_t span;
    source_loc_t source_loc;
    char *msg;
} error_t;

typedef array_t(error_t) errors_t;

#endif // !ERROR_H