#ifndef COMMON_MACROS_H
#define COMMON_MACROS_H

#if !defined(RESIZEBUFFER_CUSTOM_ERROR)
#define RESIZEBUFFER_CUSTOM_ERROR 0
#endif

#if !defined(CHECKED_ARITHMETIC)
#define CHECKED_ARITHMETIC 0
#endif

#if RESIZEBUFFER_CUSTOM_ERROR
#define resizebuffer(old_buffer, newsize, custom_error, with_free)                            \
    void* new_buffer = realloc(old_buffer, newsize);                                          \
    if (new_buffer == NULL) {                                                                 \
        error("realloc(): Unknown reason.");                                                  \
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

#define SUCCESS 0
#define FAILURE 1
#define NEGFAILURE -1

// handles platform quirks
#if HAVE_WINSOCK2_H
#define PLATFORM_REP_SOCKET SOCKET
#define PLATFORM_NETWORK_HEADER <winsock2.h>
#define PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE "%i"
#define PLATFORM_NETWORK_GET_ERROR WSAGetLastError()
#define PLATFORM_NETWORK_REP_SOCKET_ERROR SOCKET_ERROR
#define PLATFORM_NETWORK_REP_INVALID_SOCKET INVALID_SOCKET
#define PLATFORM_NETWORK_RECV_REP_SOCKET_ERROR == SOCKET_ERROR
#elif HAVE_SYS_SOCKET_H
#define PLATFORM_REP_SOCKET int
#define PLATFORM_NETWORK_HEADER <sys/socket.h>
#define PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE "%s"
#define PLATFORM_NETWORK_GET_ERROR strerror(errno)
#define PLATFORM_NETWORK_REP_SOCKET_ERROR NEGFAILURE
#define PLATFORM_NETWORK_REP_INVALID_SOCKET NEGFAILURE
#define PLATFORM_NETWORK_RECV_REP_SOCKET_ERROR <= 0
#else
#error No implementation found for networking!
#endif
#endif
