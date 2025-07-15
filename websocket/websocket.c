#include <arpa/inet.h>
#include "websocket.h"
#include "check.h"

int websocket_connect(struct parsed_url* purl) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    check(fd < 0);
    
    struct in_addr addr;
    check(inet_aton(purl->address, &addr) < 0);
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(purl->port);
    serverAddress.sin_addr.s_addr = addr.s_addr;
    
    check2(connect(fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0);

    // tell the server to upgrade the connection 
    char message[1024] = ""; 
    make_http_header(purl, message);

    printf("Sending message: %s\n", message);
    check(send(fd, message, strlen(message), 0) < 0);
    
    return fd;
}

void make_http_header(struct parsed_url* url, char* message) {
    strcat(message, "GET ");
    strcat(message, url->path);
    strcat(message, " HTTP/1.1\n");

    // Host:
    strcat(message, "Host: ");
    strcat(message, url->address);
    strcat(message, "\n");\

    // User Agent:
    strcat(message, "User-Agent: wsp-websocket/1.0\n");

    // Accept:
    strcat(message, "Accept: */*\n");

    // Upgrade: 
    // strcat(message, "Upgrade: websocket\n");

    // Connection:
    // strcat(message, "Connection: Upgrade\n");

    // strcat(message, "Sec-WebSocket-Protocol: stream\n");

    // Blank Line (end of request)
    strcat(message, "\n");
}

// i don't really know where to find info so 
// https://en.wikipedia.org/wiki/WebSocket#Protocol
// https://datatracker.ietf.org/doc/html/rfc6455