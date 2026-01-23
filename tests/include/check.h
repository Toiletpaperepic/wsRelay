#ifndef CHECK_CUSTOM_ERROR_HANDLE
#define CHECK_CUSTOM_ERROR_HANDLE 0
#endif

#if CHECK_CUSTOM_ERROR_HANDLE
#define check(expression, custom_error_handle)                                                       \
    if (expression) {                                                                                \
                                                                                                     \
    } else {                                                                                         \
        fprintf(stderr, "check \"%s\" failed on %s:%i\n", #expression, __FILE__, __LINE__);          \
        custom_error_handle                                                                          \
    }                                                                                                \

#define rcheck(expression, reason, custom_error_handle)                                                                  \
    if (expression){                                                                                                     \
                                                                                                                         \
    } else {                                                                                                             \
        fprintf(stderr, "check \"%s\" failed on %s:%i, reason: %s\n", #expression, __FILE__, __LINE__, reason);          \
        custom_error_handle                                                                                              \
    }                                                                                                                    \

#else
#define check(expression)                                                                            \
    if (expression) {                                                                                \
                                                                                                     \
    } else {                                                                                         \
        fprintf(stderr, "check \"%s\" failed on %s:%i\n", #expression, __FILE__, __LINE__);          \
        return 1;                                                                                    \
    }                                                                                                \

#define rcheck(expression, reason)                                                                                       \
    if (expression){                                                                                                     \
                                                                                                                         \
    } else {                                                                                                             \
        fprintf(stderr, "check \"%s\" failed on %s:%i, reason: %s\n", #expression, __FILE__, __LINE__, reason);          \
        return 1;                                                                                                        \
    }                                                                                                                    \

#endif