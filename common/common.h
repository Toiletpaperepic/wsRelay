#include <stdbool.h>
#include <stdio.h>

struct allowed_log_types {
    bool error;
    bool warn;
    bool info;
    bool debug;
    bool trace;
};

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

void setuplogger();
struct allowed_log_types* getalt();

#define print(format, ...) printf(format "\n", ##__VA_ARGS__);
#define stderrprint(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__);

#define ERROR(format, ...) if(getalt()->error) {stderrprint(RED "[error] " RESET format, ##__VA_ARGS__)}
#define WARN(format, ...) if(getalt()->warn) {stderrprint(YEL "[warn] " RESET format, ##__VA_ARGS__)}
#define INFO(format, ...) if(getalt()->info) {stderrprint(BLU "[info] " RESET format, ##__VA_ARGS__)}
#define DEBUG(format, ...) if(getalt()->debug) {stderrprint(CYN "[debug] " RESET format, ##__VA_ARGS__)}
#define TRACE(format, ...) if(getalt()->trace) {stderrprint(MAG "[trace] " RESET format, ##__VA_ARGS__)}

#if !defined(RESIZEBUFFER_CUSTOM_ERROR)
#define RESIZEBUFFER_CUSTOM_ERROR 0
#endif

#if !defined(CHECKED_ARITHMETIC)
#define CHECKED_ARITHMETIC 0
#endif

#if !defined(STRING_TO_INT_CONVERSION)
#define STRING_TO_INT_CONVERSION 0
#endif

#if RESIZEBUFFER_CUSTOM_ERROR
#define resizebuffer(old_buffer, newsize, custom_error, with_free)                            \
    void* new_buffer = realloc(old_buffer, newsize);                                          \
    if (new_buffer == NULL) {                                                                 \
        ERROR("realloc(): Unknown reason.");                                                  \
        if (with_free) {                                                                      \
            free(old_buffer);                                                                 \
        }                                                                                     \
        custom_error                                                                          \
    } else if (old_buffer != new_buffer) {                                                    \
        old_buffer = new_buffer;                                                              \
    }                                                                                         \
    new_buffer = NULL;
#else
#define resizebuffer(old_buffer, newsize)                                                     \
    void* new_buffer = realloc(old_buffer, newsize);                                          \
    assert(new_buffer != NULL);                                                               \
    if (old_buffer != new_buffer) {                                                           \
        old_buffer = new_buffer;                                                              \
    }                                                                                         \
    new_buffer = NULL;
#endif

#if CHECKED_ARITHMETIC
#if defined(HAVE_STDCKDINT_H)
#include <stdckdint.h>
#define CHECKED_ADD(R, A, B) ckd_add((R), (A), (B))
#define CHECKED_MUL(R, A, B) ckd_mul((R), (A), (B))
#elif defined(HAVE___BUILTIN_ADD_OVERFLOW) && defined(HAVE___BUILTIN_ADD_OVERFLOW)
#define CHECKED_ADD(R, A, B) __builtin_add_overflow((A), (B), (R))
#define CHECKED_MUL(R, A, B) __builtin_mul_overflow((A), (B), (R))
#else
#include <jtckdint.h>
#define CHECKED_ADD(R, A, B) ckd_add((R), (A), (B))
#define CHECKED_MUL(R, A, B) ckd_mul((R), (A), (B))
#endif
#endif

#if STRING_TO_INT_CONVERSION
// bool strtoint16(int16_t* result, const char* str);
bool strtouint16(uint16_t* result, const char* str);
bool strtoint32(int32_t* result, const char* str);
bool strtouint32(uint32_t* result, const char* str);
#endif

#define SUCCESS 0
#define FAILURE 1
#define NEGFAILURE -1