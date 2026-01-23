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
