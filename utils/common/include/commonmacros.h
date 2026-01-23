#ifndef RESIZEBUFFER_CUSTOM_ERROR
#define RESIZEBUFFER_CUSTOM_ERROR 0
#endif

#if RESIZEBUFFER_CUSTOM_ERROR
#define resizebuffer(old_buffer, newsize, custom_error)                                       \
    void* new_buffer = realloc(old_buffer, newsize);                                          \
    if (new_buffer == NULL) {                                                                 \
        fprintf(stderr, "realloc(): Unknown reason.\n");                                      \
        free(old_buffer);                                                                     \
        custom_error                                                                          \
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
