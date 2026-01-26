#include <sys/random.h>
#include <arpa/inet.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include "commonmacros.h"
#include "websocket.h"
#include "http_header.h"

// https://en.wikipedia.org/wiki/WebSocket#Protocol

#if defined(__ANDROID__) && __ANDROID_API__ < 28
#include <sys/syscall.h>
#include <unistd.h>
#define getrandom(buf,buflen,flags) syscall(SYS_getrandom,buf,buflen,flags)
#endif

int websocket_connect(struct parsed_url purl) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "socket(): %s.\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(purl.port);

    if (inet_pton(AF_INET, purl.address, &serverAddress.sin_addr) < 0) {
        fprintf(stderr, "inet_aton(): failed, Invalid address.\n");
        return -1;
    }
    
    if (connect(fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "connect(): %s.\n", strerror(errno));
        return -1;
    }

    // tell the server to upgrade the connection 
    const char* message = make_http_header(purl);
    printf("Sending message: %s\n", message);

    if (send(fd, message, strlen(message), 0) < 0) {
        fprintf(stderr, "send(): %s.\n", strerror(errno));
        return -1;
    }

    free((void*)message);

    char buffer[1024] = {};
    ssize_t size = recv(fd, buffer, sizeof(buffer), 0);
    if (size < 0) {
        fprintf(stderr, "recv(): %s.\n", strerror(errno));
        return -1;
    }
    printf("received accept message with size of %zi, %s\n", size, buffer);
    
    //TODO: check for a valid response?

    return fd;
}

void websocket_send(int fd, void* buffer, uint64_t size, enum opcodes opcode, bool FIN) {
    uint8_t byte0 = 0, byte1 = 0;

    if(FIN == true) {
        byte0 = byte0 | 0b10000000;
    } /* else {
        byte0 = byte0 | 0b00000000;
    } */
    
    byte0 = byte0 | opcode;

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

    if (buffer == NULL && size == 0) {
        printf("no payload provided.\n");
    } else {
        memcpy(payload + 2 + extraPayloadlength + sizeof(maskingkey), buffer, size);

        printf("payload: ");
        for (int i = 0; i < size; i++) {
            printf("%X ", payload[2 + extraPayloadlength + sizeof(maskingkey) + i]);
        }
        printf("\n");
        
        printf("payload (size): %lu\n", sizeof(payload));

        printf("payload (masked): ");
        for (int i = 0; i < size; i++) {
            payload[2 + extraPayloadlength + sizeof(maskingkey) + i] = payload[2 + extraPayloadlength + sizeof(maskingkey) + i] ^ maskingkey[i % 4];
            printf("%X ", payload[2 + extraPayloadlength + sizeof(maskingkey) + i]);
        }
        printf("\n");
    }

    if (send(fd, payload, sizeof(payload), 0) < 0) {
        fprintf(stderr, "send(): %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

struct message websocket_recv(int fd) {
    bool FIN = false;

    struct message msg;
    memset(&msg, 0, sizeof(struct message));
    msg.buffer = NULL;
    
    while (FIN != true) {
        uint8_t header[2] = {};
        if (recv(fd, header, sizeof(header), MSG_WAITALL) < 0) {
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        FIN = (header[0] & 0b10000000) != 0;
        printf("FIN: %s\n", FIN ? "True" : "False");

        assert((header[0] & 0b01110000) == 0); // fixme: we should really disconect instead of crashing the program.

        enum opcodes opcode = header[0] & 0b00001111;
        printf("opcode: %i\n", opcode);

        bool masked = (header[1] & 0b10000000) != 0;
        printf("masked: %i\n", masked);
        assert(masked == false);

        uint64_t payload_size = header[1] & 0b01111111;

        if (payload_size == 126) {
            if (recv(fd, &payload_size, sizeof(uint16_t), 0) < 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            payload_size = be16toh(payload_size);
        }
        else if (payload_size == 127) {
            if (recv(fd, &payload_size, sizeof(uint64_t), 0) < 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            payload_size = be64toh(payload_size);
        }

        printf("payload size: %lu, current buffer size: %lu\n", payload_size, msg.size);

        if (payload_size > 0) {
            if (msg.buffer == NULL) {
                msg.buffer = malloc(payload_size);
            } else {
                printf("resizing buffer... %lu -> %lu\n", msg.size, msg.size + payload_size);
                resizebuffer(msg.buffer, msg.size + payload_size);
            }
            
            if (recv(fd, msg.buffer + msg.size, payload_size, MSG_WAITALL) < 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                free(msg.buffer);
                exit(EXIT_FAILURE);
            }

            printf("payload: ");
            for (int i = 0; i < payload_size; i++) {
                printf("%X ", *(uint8_t *)(msg.buffer + msg.size + i));
            }
            printf("\n");
        }

        msg.size += payload_size;
        msg.opcode = opcode;
    }

    // add the end string char.
    if (msg.opcode == TEXT) {
        resizebuffer(msg.buffer, msg.size + 1);
        printf("resizing buffer... %lu -> %lu\n", msg.size, msg.size + 1);

        char endchar = '\0';
        memcpy(msg.buffer + msg.size, &endchar, 1);
    }

    return msg;
}
