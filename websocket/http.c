#include <sys/random.h>
#include <base64.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "common_macros.h"
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

/// going for something like this -> "wsrRelay/v1.0 (Linux; gcc 15.2.1; x86_64; +https://github.com/Toiletpaperepic/wsRelay/) Hostname/Apollo-Lake"
void make_user_agent(char** destinationstring) {
    appendchar(destinationstring, "User-Agent: wsrRelay/v");
    appendchar(destinationstring, __PROJECT_VERSION__);
    appendchar(destinationstring, " ("); 
    
    // TODO: create USER_AGENT_LESS_INFO env
    #if __ANDROID__
    appendchar(destinationstring, "Android");
    #elif __linux__
    appendchar(destinationstring, "Linux");
    #elif __WIN32__
    appendchar(destinationstring, "Windows");
    #else
    #warning unknown platform
    appendchar(destinationstring, "unknown platform");
    #endif
    
    appendchar(destinationstring, ";");
    
    #if defined(__clang__)
    appendchar(destinationstring, " clang ");
    appendchar(destinationstring, __clang_version__);
    #elif defined(__GNUC__)
    appendchar(destinationstring, " gcc ");
    appendchar(destinationstring, __VERSION__);
    #else
    #warning unknown compiler
    appendchar(destinationstring, " unknown compiler");
    #endif
    
    appendchar(destinationstring, ";");
    
    #if __x86_64__
    appendchar(destinationstring, " x86_64");
    #elif __aarch64__
    appendchar(destinationstring, " aarch64");
    #else
    #warning unknown arch
    appendchar(destinationstring, " unknown arch");
    #endif
    
    appendchar(destinationstring, "; +https://github.com/Toiletpaperepic/wsRelay/) ");
    
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        fprintf(stderr, "gethostname(): %s.\n", strerror(errno));
    } else {
    
        appendchar(destinationstring, "Hostname/");
        appendchar(destinationstring, hostname);
    }
    
    appendchar(destinationstring, "\n");
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
    make_user_agent(&message);

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