#include <sys/random.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
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

void websocket_send(struct connection con, void* buffer, uint64_t size) {
    uint8_t byte0 = 0;
    uint8_t byte1 = 0;
    bool FIN = true;

    if(FIN == true) {
        byte0 = byte0 | 0b10000000;
    } else {
        // byte0 = byte0 | 0b00000000;
    }

    // enum opcodes opcode = TEXT;
    enum opcodes opcode = BINARY;

    switch (opcode) {
        case CONTINUATION:
            byte0 = byte0 | CONTINUATION;
            break;
        case TEXT:
            byte0 = byte0 | TEXT;
            break;
        case BINARY:
            byte0 = byte0 | BINARY;
            break;
        case CLOSE:
            byte0 = byte0 | CLOSE;
            break;
        case PING:
            byte0 = byte0 | PING;
            break;
        case PONG:
            byte0 = byte0 | PONG;
            break;
    }

    assert((byte0 & 0b01110000) == 0);

    // masked is always true as a client.
    byte1 = byte1 | 0b10000000;
    unsigned int extraPayloadlength = 0;

    if (size <= 125) {
        byte1 = byte1 | size;
        printf("size is smaller then 125\n");
    } else if (size >= 125 && size < UINT16_MAX) { // 16 bits
        byte1 = byte1 | 126;
        extraPayloadlength = sizeof(uint16_t);
        printf("size is smaller then UINT16_MAX\n");
    } else if (size >= 125 && size > UINT16_MAX && size < UINT64_MAX) { // 64 bits
        byte1 = byte1 | 127;
        extraPayloadlength = sizeof(uint64_t);
        printf("size is smaller then UINT64_MAX\n");
    }

    uint8_t maskingkey[4] = {};
    getrandom(&maskingkey, sizeof(maskingkey), 0);

    printf("masking key: ");
    for (int i = 0; i < sizeof(maskingkey); i++)
        printf("%X ", maskingkey[i]);
    printf("\n");

    uint8_t payload[2 + extraPayloadlength + sizeof(maskingkey) + size] = {};

    memcpy(payload, &byte0, sizeof(byte0));
    memcpy(payload + 1, &byte1, sizeof(byte1));

    if (extraPayloadlength == sizeof(uint16_t)) {
        uint16_t size_big_endian = htobe16(size);
        memcpy(payload + 2, &size_big_endian, extraPayloadlength);
    } else if (extraPayloadlength == sizeof(uint64_t)) {
        uint64_t size_big_endian = htobe64(size);
        memcpy(payload + 2, &size_big_endian, extraPayloadlength);
    }

    memcpy(payload + 2 + extraPayloadlength, maskingkey, sizeof(maskingkey));
    memcpy(payload + 2 + extraPayloadlength + sizeof(maskingkey), buffer, size);

    printf("payload (masked): ");
    for (int i = 0; i < size; i++) {
        payload[2 + extraPayloadlength + sizeof(maskingkey) + i] = payload[2 + extraPayloadlength + sizeof(maskingkey) + i] ^ maskingkey[i % 4];
        printf("%X ", payload[2 + extraPayloadlength + sizeof(maskingkey) + i]);
    }
    printf("\n");

    printf("payload (size): %lu\n", sizeof(payload));

    if (send(con.fd, payload, sizeof(payload), 0) < 0) {
        fprintf(stderr, "send(): %s.\n", strerror(errno));
        exit(errno);
    }
}

struct message websocket_recv(struct connection con) {
    bool FIN = false;
    enum opcodes opcode;
    uint64_t payload_size;

    struct message msg;
    msg.buffer = NULL;
    
    while (FIN != true) {
        uint8_t header[2] = {};
        if (recv(con.fd, header, sizeof(header), 0) < 0) {
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            exit(errno);
        }

        FIN = (header[0] & 0b10000000) != 0;
        printf("FIN: %i\n", FIN);

        assert((header[0] & 0b01110000) == 0);

        enum opcodes opcode = header[0] & 0b00001111;
        printf("opcode: %i\n", opcode);
        assert(opcode == TEXT || opcode == BINARY);

        bool masked = (header[1] & 0b10000000) != 0;
        printf("masked: %i\n", masked);
        assert(masked == false);

        uint64_t payload_size = header[1] & 0b01111111;
        unsigned int extraPayloadlength = 0;

        if (payload_size == 126) {
            if (recv(con.fd, &payload_size, sizeof(uint16_t), 0) < 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                exit(errno);
            }
            payload_size = be16toh(payload_size);
        }
        else if (payload_size == 127) {
            if (recv(con.fd, &payload_size, sizeof(uint64_t), 0) < 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                exit(errno);
            }
            payload_size = be64toh(payload_size);
        }

        printf("payload size: %lu\n", payload_size);

        if (msg.buffer == NULL) {
            msg.buffer = malloc(payload_size);
        } else {
            uint64_t newsize = msg.size + payload_size;
            printf("resizing buffer... %lu -> %lu\n", msg.size, newsize);
            void* new_buffer = realloc(msg.buffer, newsize);

            if (new_buffer == NULL) {
                fprintf(stderr, "realloc(): Unknown reason.");
                free(msg.buffer);
            } else {
                msg.buffer = new_buffer;
            }
        }
        
        if (recv(con.fd, msg.buffer + msg.size, payload_size, 0) < 0) {
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            free(msg.buffer);
            exit(errno);
        }

        msg.size += payload_size;
        msg.opcodes = opcode;
    }
    
    return msg;
}
