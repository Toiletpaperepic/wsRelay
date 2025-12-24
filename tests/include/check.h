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

