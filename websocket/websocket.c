#include <openssl/sha.h>
#include <arpa/inet.h>
#include <base64-cstring.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "websocket.h"
#include "check.h"

void make_http_header(const char* path, const char* address, char* message) {
    strcat(message, "GET ");
    strcat(message, path);
    strcat(message, " HTTP/1.1\n");

    // Host:
    strcat(message, "Host: ");
    strcat(message, address);
    strcat(message, "\n");

    // User Agent:
    strcat(message, "User-Agent: wsp-websocket/1.0\n");

    // Accept:
    strcat(message, "Accept: */*\n");

    // Upgrade: 
    strcat(message, "Upgrade: websocket\n");

    // Connection:
    strcat(message, "Connection: Upgrade\n");

    // WebSocket Version: 
    strcat(message, "Sec-WebSocket-Version: 13\n");

    char key[1024] = "";
    int nonce = rand();
    // i don't know whats worse, hard coding or this.
    char src[(int)floor (log10 (abs (nonce))) + 1 + 1]; 
    sprintf(src, "%d", nonce);
    base64_encode_c(key, src);
    
    // WebSocket Key: 
    strcat(message, "Sec-WebSocket-Key: ");
    strcat(message, key);
    strcat(message, "\n");
    
    // Blank Line (end of request)
    strcat(message, "\n");
}

int websocket_connect(struct parsed_url* purl) {
    // for some reason purl->address and purl->path disappear when "make_http_header" is called. why...
    // copy both strings before they disappear
    char address[strlen(purl->address)];
    char path[strlen(purl->path)];
    strcpy (address,purl->address);
    strcpy (path,purl->path);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    check2(fd < 0);
    
    struct in_addr addr;
    check2(inet_aton(address, &addr) < 0);
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(purl->port);
    serverAddress.sin_addr.s_addr = addr.s_addr;
    
    check2(connect(fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0);

    // tell the server to upgrade the connection 
    char message[1024] = ""; 
    make_http_header(path, address, message);

    printf("Sending message: %s\n", message);
    check(send(fd, message, strlen(message), 0) < 0);
    
    return fd;
}

// i don't really know where to find info so 
// https://en.wikipedia.org/wiki/WebSocket#Protocol