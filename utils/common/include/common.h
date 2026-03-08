#ifndef RESIZEBUFFER_CUSTOM_ERROR
#define RESIZEBUFFER_CUSTOM_ERROR 0
#endif

#if RESIZEBUFFER_CUSTOM_ERROR
#define resizebuffer(old_buffer, newsize, custom_error)                                       \
    void* new_buffer = realloc(old_buffer, newsize);                                          \
    if (new_buffer == NULL) {                                                                 \
        fprintf(stderr, "realloc(): Unknown reason.\n");                                      \
        custom_error                                                                          \
        free(old_buffer);                                                                     \
    } else if (old_buffer != new_buffer) {                                                    \
        old_buffer = new_buffer;                                                              \
    }                                                                                         \
    new_buffer = NULL;                                                                        \

#else
#define resizebuffer(old_buffer, newsize)                                                     \
    void* new_buffer = realloc(old_buffer, newsize);                                          \
    if (new_buffer == NULL) {                                                                 \
        fprintf(stderr, "realloc(): Unknown reason.\n");                                      \
        free(old_buffer);                                                                     \
        exit(EXIT_FAILURE);                                                                   \
    } else if (old_buffer != new_buffer) {                                                    \
        old_buffer = new_buffer;                                                              \
    }                                                                                         \
    new_buffer = NULL;                                                                        \

#endif

#if defined(HAVE_STDCKDINT_H)
#define CHECKED_ADD(R, A, B) ckd_add((R), (A), (B))
#elif defined(HAVE___BUILTIN_ADD_OVERFLOW)
#define CHECKED_ADD(R, A, B) __builtin_add_overflow(A, B, R)
#else
#error Missing implementation for Checked Arithmetic for addition!
#endif

#if defined(HAVE_STDCKDINT_H)
#define CHECKED_MUL(R, A, B) ckd_mul((R), (A), (B))
#elif defined(HAVE___BUILTIN_ADD_OVERFLOW)
#define CHECKED_MUL(R, A, B) __builtin_mul_overflow((A), (B), (R))
#else
#error Missing implementation for Checked Arithmetic for multiplication!
#endif