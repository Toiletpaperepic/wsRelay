#include <errno.h>
#include <sys/random.h>
#include <base64.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include "commonmacros.h"
#include "websocket.h"

#if defined(__ANDROID__) && __ANDROID_API__ < 28
#include <sys/syscall.h>
#include <unistd.h>
#define getrandom(buf,buflen,flags) syscall(SYS_getrandom,buf,buflen,flags)
#endif

void appendchar(char** destinationstring, const char* sourcestring) { 
    resizebuffer(*destinationstring, strlen(*destinationstring) + strlen(sourcestring) + 1); 
    strcat(*destinationstring, sourcestring);
} 

const char* make_http_header(struct parsed_url purl) {
    char* message = malloc(1);
    message[0] = '\0';

    appendchar(&message, "GET ");
    appendchar(&message, purl.path);
    appendchar(&message, " HTTP/1.1\n");

    // Host:
    appendchar(&message, "Host: ");
    appendchar(&message, purl.address);
    appendchar(&message, "\n");

    
    // User Agent:
    /// going for something like this -> "wsrRelay/v1.0 (Linux; gcc 15.2.1; x86_64; https://github.com/Toiletpaperepic/wsRelay/) Hostname/Apollo-Lake"
    appendchar(&message, "User-Agent: wsrRelay/v");
    appendchar(&message, "1.0 (" /* TODO: get this program's version from cmake */); 

    // TODO: create USER_AGENT_LESS_INFO env

#if __linux__
    appendchar(&message, "Linux");
#elif __WIN32__
    appendchar(&message, "Windows");
#else
#warning unknown platform
appendchar(&message, "unknown platform");
#endif

    appendchar(&message, ";");

#if defined(__clang__)
    appendchar(&message, " clang ");
    appendchar(&message, __clang_version__);
    message[strlen(message) - 1] = ';';
#elif defined(__GNUC__)
    appendchar(&message, " gcc ");
    appendchar(&message, __VERSION__);
    appendchar(&message, ";");
#else
#warning unknown compiler
    appendchar(&message, " unknown compiler;");
#endif

#if __x86_64__
    appendchar(&message, " x86_64");
#elif __aarch64__
    appendchar(&message, " aarch64");
#else
#warning unknown arch
    appendchar(&message, " unknown arch");
#endif

    appendchar(&message, "; +https://github.com/Toiletpaperepic/wsRelay/) ");

    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        fprintf(stderr, "gethostname(): %s.\n", strerror(errno));
    } else {

        appendchar(&message, "Hostname/");
        appendchar(&message, hostname);
    }

    appendchar(&message, "\n");

    // Accept:
    appendchar(&message, "Accept: */*\n");

    // Upgrade: 
    appendchar(&message, "Upgrade: websocket\n");

    // Connection:
    appendchar(&message, "Connection: Upgrade\n");

    // WebSocket Version: 
    appendchar(&message, "Sec-WebSocket-Version: 13\n");

    uint8_t nonce[16];
    getrandom(&nonce, sizeof(nonce), 0);
    const char* key = base64_encode_no_lf(&nonce, sizeof(nonce), NULL);

    assert(strlen(key) == 24);
    
    // WebSocket Key: 
    appendchar(&message, "Sec-WebSocket-Key: ");
    appendchar(&message, key);
    appendchar(&message, "\n");

    free((void*)key);

    // Blank Line (end of request)
    appendchar(&message, "\n");

    return message;
}