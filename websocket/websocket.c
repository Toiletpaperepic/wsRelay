#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include "websocket.h"
#include "https_headers.h"

// https://en.wikipedia.org/wiki/WebSocket#Protocol

#if defined(__ANDROID__) && __ANDROID_API__ < 28
#include <sys/syscall.h>
#include <unistd.h>
#define getrandom(buf,buflen,flags) syscall(SYS_getrandom,buf,buflen,flags)
#endif

struct connection websocket_connect(struct parsed_url purl) {
    struct connection con;
    con.url = purl;
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "socket(): %s.\n", strerror(errno));
        exit(errno);
    }
    con.fd = fd;
    
    struct in_addr addr;
    if (inet_aton(con.url.address, &addr) < 0) {
        fprintf(stderr, "inet_aton(): failed, Invalid address.\n");
    }
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(con.url.port);
    serverAddress.sin_addr.s_addr = addr.s_addr;
    
    if (connect(fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "connect(): %s.\n", strerror(errno));
        exit(errno);
    }

    // tell the server to upgrade the connection 
    char message[1024] = ""; 
    make_http_header(con.url, message);
    printf("Sending message: %s\n", message);

    if (send(con.fd, message, strlen(message), 0) < 0) {
        fprintf(stderr, "send(): %s.\n", strerror(errno));
        exit(errno);
    }

    char buffer[1024] = {};
    if (recv(con.fd, buffer, sizeof(buffer), 0) < 0) {
        fprintf(stderr, "recv(): %s.\n", strerror(errno));
        exit(errno);
    }
    printf("received accept message: %s\n", buffer);
    memset(buffer, '\0', sizeof(buffer));

    //TODO: check for a valid response?

    return con;
}

void websocket_send(struct connection con, void* buffer, size_t size) {
    uint8_t payload[2 + 4 + size] = {};
    bool FIN = true;

    if(FIN == true) {
        payload[0] = payload[0] | 0b10000000;
    } else {
        // payload[0] = payload[0] | 0b00000000;
    }

    // enum opcodes opcode = TEXT;
    enum opcodes opcode = BINARY;

    switch (opcode) {
        case CONTINUATION:
            payload[0] = payload[0] | CONTINUATION;
            break;
        case TEXT:
            payload[0] = payload[0] | TEXT;
            break;
        case BINARY:
            payload[0] = payload[0] | BINARY;
            break;
        case CLOSE:
            payload[0] = payload[0] | CLOSE;
            break;
        case PING:
            payload[0] = payload[0] | PING;
            break;
        case PONG:
            payload[0] = payload[0] | PONG;
            break;
    }

    assert((payload[0] & 0b01110000) == 0);

    // masked is always true as a client.
    payload[1] = payload[1] | 0b10000000;

    assert(size < 127);
    payload[1] = payload[1] | size;

    uint8_t maskingkey[4] = {};
    getrandom(&maskingkey, sizeof(maskingkey), 0);
    memcpy(&payload[2], &maskingkey, sizeof(maskingkey));
    memcpy(&payload[6], buffer, size);
    
    for (int i = 0; i < size; i++) {
        payload[6 + i] = payload[6 + i] ^ maskingkey[i % 4];
    }

    if (send(con.fd, payload, sizeof(payload), 0) < 0) {
        fprintf(stderr, "send(): %s.\n", strerror(errno));
        exit(errno);
    }
}

struct message websocket_recv(struct connection con) {
    uint8_t header[2] = {};
    if (recv(con.fd, header, sizeof(header), 0) < 0) {
        fprintf(stderr, "recv(): %s.\n", strerror(errno));
        exit(errno);
    }

    int FIN = (header[0] & 0b10000000) != 0;
    printf("FIN: %i\n", FIN);
    assert(FIN == 1);

    assert((header[0] & 0b01110000) == 0);

    enum opcodes opcode = header[0] & 0b00001111;
    printf("opcode: %i\n", opcode);
    assert(opcode == TEXT || opcode == BINARY);

    int masked = (header[1] & 0b10000000) != 0;
    printf("masked: %i\n", masked);
    assert(masked == 0);

    unsigned int payload_size = header[1] & 0b01111111;
    printf("payload size: %i\n", payload_size);

    memset(header, '\0', sizeof(header));

    struct message msg;
    msg.size = payload_size;
    msg.buffer = malloc(payload_size);

    if (recv(con.fd, msg.buffer, payload_size, 0) < 0) {
        fprintf(stderr, "recv(): %s.\n", strerror(errno));
        exit(errno);
    }

    return msg;
}
