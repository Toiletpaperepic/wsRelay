#ifdef __WIN32__
#include <windows.h>
#include <stdio.h>
#elif defined(__ANDROID__) && __ANDROID_API__ < 28
#include <sys/syscall.h>
#include <unistd.h>
#define getrandom(buf,buflen,flags) syscall(SYS_getrandom,buf,buflen,flags)
#else
#include <sys/random.h>
#endif
#include <base64.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "websocket.h"
#include "base64.h"


void make_http_header(struct parsed_url purl, char* message) {
    strcat(message, "GET ");
    strcat(message, purl.path);
    strcat(message, " HTTP/1.1\n");

    // Host:
    strcat(message, "Host: ");
    strcat(message, purl.address);
    strcat(message, "\n");

    // User Agent:
    strcat(message, "User-Agent: wsr-websocket/1.0\n");

    // Accept:
    strcat(message, "Accept: */*\n");

    // Upgrade: 
    strcat(message, "Upgrade: websocket\n");

    // Connection:
    strcat(message, "Connection: Upgrade\n");

    // WebSocket Version: 
    strcat(message, "Sec-WebSocket-Version: 13\n");

    uint8_t nonce[16];
#if __WIN32__
    BCRYPT_ALG_HANDLE handle;
    NTSTATUS error;

    error = BCryptOpenAlgorithmProvider(&handle, BCRYPT_RNG_ALGORITHM,NULL,0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptOpenAlgorithmProvider(): %lX.\n", error);
        exit(EXIT_FAILURE);
    }
    
    error = BCryptGenRandom(&handle, (unsigned char*)nonce, sizeof(nonce), 0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptGenRandom(): %lX.\n", error);
        exit(EXIT_FAILURE);
    }
    
    error = BCryptCloseAlgorithmProvider(handle,0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptCloseAlgorithmProvider(): %lX.\n", error);
        exit(EXIT_FAILURE);
    }
#else
    getrandom(&nonce, sizeof(nonce), 0);
#endif
    const char* key = base64_encode_no_lf(&nonce, sizeof(nonce), NULL);

    assert(strlen(key) == 24);
    
    // WebSocket Key: 
    strcat(message, "Sec-WebSocket-Key: ");
    strcat(message, key);
    strcat(message, "\n");
    
    free((void*)key);

    // Blank Line (end of request)
    strcat(message, "\n");
}