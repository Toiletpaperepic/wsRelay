#include <sys/random.h>
#include <base64.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
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
    appendchar(&message, "User-Agent: wsr-websocket/1.0\n");

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