#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/// don't exit on error
#define check(expression)   \
    {   \
        int result = expression;    \
        if (result) {    \
            printf("Error: failed on %s:%i, got %i, reason: %s\n", __FILE__, __LINE__, result, strerror(errno));  \
        }   \
    }

/// exit on error
#define check2(expression)   \
    {   \
        int result = expression;    \
        if (result) {    \
            printf("Error: failed on %s:%i, got %i, reason: %s\n", __FILE__, __LINE__, result, strerror(errno));  \
            exit(1);    \
        }   \
    }