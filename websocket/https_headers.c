#include <base64.h>
#include <sys/random.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "websocket.h"

#if defined(__ANDROID__) && __ANDROID_API__ < 28
#include <sys/syscall.h>
#include <unistd.h>
#define getrandom(buf,buflen,flags) syscall(SYS_getrandom,buf,buflen,flags)
#endif

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

    char nonce[16 + 1];
    getrandom(&nonce, sizeof(nonce), 0);

    for (int i = 0; i < sizeof(nonce); i++) {
        while (nonce[i] == '\0') {
            getrandom(&nonce[i], sizeof(char), 0);
        }
    }

    nonce[sizeof(nonce) - 1] = '\0';
    const char* key = base64_encode_no_lf(&nonce, sizeof(nonce) - 1, NULL);

    assert(strlen(key) == 24);
    
    // WebSocket Key: 
    strcat(message, "Sec-WebSocket-Key: ");
    strcat(message, key);
    strcat(message, "\n");
    
    free((void*)key);

    // Blank Line (end of request)
    strcat(message, "\n");
}