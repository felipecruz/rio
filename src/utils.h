#include <stdio.h>

#ifndef __UTILS_H_
#define __UTILS_H_

#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#define new(var, type)                \
    var = malloc(sizeof(type));       \
    if (var == NULL) {                \
        error_exit("malloc error");   \
    }

void error_exit(const char *message);
#endif
