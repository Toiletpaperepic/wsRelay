#include <sys/random.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "websocket.h"
#include "https_headers.h"

#define PAYLOAD_MAX 125

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
    for (int i = 1; i <= size; i++) {
        if (i % PAYLOAD_MAX == 0 || i == size) {
            int payloadsize = ((i - 1) % PAYLOAD_MAX) + 1;
            uint8_t payload[2 + 4 + payloadsize] = {};
            
            printf("Payload size: 2 + 4 + %d = %lu\n", payloadsize, sizeof(payload));

            bool FIN = i != 0;
            if(FIN == true) {
                payload[0] = payload[0] | 0b10000000;
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
        
            assert(payloadsize <= PAYLOAD_MAX);
            payload[1] = payload[1] | payloadsize;
        
            uint8_t maskingkey[4] = {};
            getrandom(&maskingkey, sizeof(maskingkey), 0);
        
            printf("masking key: ");
            for (int i = 0; i < sizeof(maskingkey); i++)
                printf("%X ", maskingkey[i]);
            printf("\n");
        
            memcpy(payload + 2, &maskingkey, sizeof(maskingkey));
            memcpy(payload + 6, buffer + (i - payloadsize), payloadsize);
        
            printf("payload (masked): ");
            for (int i = 0; i < payloadsize; i++) {
                payload[6 + i] = payload[6 + i] ^ maskingkey[i % 4];
                printf("%X ", payload[6 + i]);
            }
            printf("\n");
        
            if (send(con.fd, payload, sizeof(payload), 0) < 0) {
                fprintf(stderr, "send(): %s.\n", strerror(errno));
                exit(errno);
            }
        }
    }
}

struct message websocket_recv(struct connection con) {
    uint8_t header[2] = {};
    if (recv(con.fd, header, sizeof(header), 0) < 0) {
        fprintf(stderr, "recv(): %s.\n", strerror(errno));
        exit(errno);
    }

    bool FIN = (header[0] & 0b10000000) != 0;
    printf("FIN: %i\n", FIN);
    assert(FIN == 1);

    assert((header[0] & 0b01110000) == 0);

    enum opcodes opcode = header[0] & 0b00001111;
    printf("opcode: %i\n", opcode);
    assert(opcode == TEXT || opcode == BINARY);

    bool masked = (header[1] & 0b10000000) != 0;
    printf("masked: %i\n", masked);
    assert(masked == 0);

    uint64_t payload_size = header[1] & 0b01111111;
    if (payload_size == 126) {
        if (recv(con.fd, &payload_size, 2, 0) < 0) {
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            exit(errno);
        }
    } else if (payload_size == 127) {
        if (recv(con.fd, &payload_size, 8, 0) < 0) {
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            exit(errno);
        }
    }
    printf("payload size: %lu\n", payload_size);

    struct message msg;
    msg.size = payload_size;
    msg.buffer = malloc(payload_size);

    if (recv(con.fd, msg.buffer, payload_size, 0) < 0) {
        fprintf(stderr, "recv(): %s.\n", strerror(errno));
        exit(errno);
    }

    return msg;
}
