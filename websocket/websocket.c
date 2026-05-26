#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <io.h>
#else
#if defined(__ANDROID__) && __ANDROID_API__ < 28
#include <sys/syscall.h>
#define getrandom(buf,buflen,flags) syscall(SYS_getrandom,buf,buflen,flags)
#else
#include <sys/random.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <endian.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "wsrelay.h"
#include "common.h"

// https://en.wikipedia.org/wiki/WebSocket#Protocol

int websocket_connect(struct parsed_url purl) {
    struct addrinfo *result, *ai, hints;
    int error, fd;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // resolve the domain name into a list of addresses
    error = getaddrinfo(purl.address, NULL, &hints, &result);
    if (error != 0) {
#if !defined(_WIN32)
        if (error == EAI_SYSTEM) {
            fprintf(stderr, "getaddrinfo: %s", strerror(errno));
        } else {
            fprintf(stderr, "getaddrinfo: gai_strerror: %s\n", gai_strerror(error));
        }
#else
        fprintf(stderr, "getaddrinfo: %i", WSAGetLastError());
#endif
        return -1;
    }

    bool success = false;
    
    // loop over all returned results
    for (ai = result; ai != NULL && !success; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) {
            fprintf(stderr, "socket(): %s.\n", strerror(errno));
            continue;
        }

        if (ai->ai_family == AF_INET) {
            printf("using ipv4.\n");
            ((struct sockaddr_in*)ai->ai_addr)->sin_port = htons(purl.port);
        } else if (ai->ai_family == AF_INET6) {
            printf("using ipv6.\n");
            ((struct sockaddr_in6*)ai->ai_addr)->sin6_port = htons(purl.port);
        }
        
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) < 0) {
            fprintf(stderr, "connect(): %s.\n", strerror(errno));
            continue;
        }

        success = !success;
    }

    freeaddrinfo(result);

    if (!success)
        return NEGFAILURE;
    else
        printf("successfuly connected to a server using getaddrinfo!\n");

    // tell the server to upgrade the connection 
    const char* message = make_http_header(purl);
    printf("Sending message: %s\n", message);

    if (send(fd, message, strlen(message), 0) < 0) {
        fprintf(stderr, "send(): %s.\n", strerror(errno));
        free((void*)message);
        close(fd);
        return -1;
    }

    free((void*)message);

    struct http_response_read_result rrr = read_http_response_header(fd);
    if (rrr.error == FAILURE) 
        return NEGFAILURE;

    struct http_response_read_result_success rrrs = *(struct http_response_read_result_success*)rrr.data;
    printf("%s %s\n", getheaderfromlist("Connection", rrrs.headerslist_len, rrrs.headerslist)->header_name, getheaderfromlist("Connection", rrrs.headerslist_len, rrrs.headerslist)->header_content);

    if (strcmp(rrrs.httpcode, "101") != 0) {
        fprintf(stderr, "Server did not send a 101 response! (got %s)\n", rrrs.httpcode);
        free(rrrs.headerslist); return NEGFAILURE;
    }

    if (getheaderfromlist("Connection", rrrs.headerslist_len, rrrs.headerslist) == NULL) {
        fprintf(stderr, "Server did not send a Connection header!\n");
        free(rrrs.headerslist); return NEGFAILURE;
    }

    if (getheaderfromlist("Upgrade", rrrs.headerslist_len, rrrs.headerslist) == NULL) {
        fprintf(stderr, "Server did not send a Upgrade header!\n");
        free(rrrs.headerslist); return NEGFAILURE;
    }

    free(rrrs.headerslist);
    return fd;
}

int websocket_send(int fd, void* buffer, uint64_t size, enum opcodes opcode, bool FIN) {
    uint8_t byte0 = 0, byte1 = 0;

    if(FIN == true)
        byte0 = byte0 | 0b10000000;
    
    byte0 = byte0 | opcode;

    assert((byte0 & 0b01110000) == 0);

    byte1 = byte1 | 0b10000000; // masked is always true as a client.
    unsigned int extraPayloadlength = 0;

    if (size <= 125) { // size fits in 7 bits
        byte1 = byte1 | (uint8_t)size;
        printf("size is smaller then 125\n");
    } else if (size >= 125 && size < UINT16_MAX) { // size fits in 16 bits
        byte1 = byte1 | 126;
        extraPayloadlength = sizeof(uint16_t);
        printf("size is smaller then UINT16_MAX\n");
    } else if (size >= 125 && size > UINT16_MAX && size < UINT64_MAX) { // size fits in 64 bits
        byte1 = byte1 | 127;
        extraPayloadlength = sizeof(uint64_t);
        printf("size is smaller then UINT64_MAX\n");
    }

    uint8_t maskingkey[4];
#if defined(_WIN32)
    BCRYPT_ALG_HANDLE handle;
    NTSTATUS error;

    error = BCryptOpenAlgorithmProvider(&handle, BCRYPT_RNG_ALGORITHM,NULL,0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptOpenAlgorithmProvider(): %lX.\n", error);
        return FAILURE;
    }
    
    error = BCryptGenRandom(handle, (unsigned char*)maskingkey, sizeof(maskingkey), 0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptGenRandom(): %lX.\n", error);
        return FAILURE;
    }
    
    error = BCryptCloseAlgorithmProvider(handle,0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptCloseAlgorithmProvider(): %lX.\n", error);
        return FAILURE;
    }
#else
    getrandom(&maskingkey, sizeof(maskingkey), 0);
#endif

    printf("masking key: ");
    for (int i = 0; i < sizeof(maskingkey); i++)
        printf("%X ", maskingkey[i]);
    printf("\n");

    size_t payload_size = 2 + extraPayloadlength + sizeof(maskingkey) + size;
    uint8_t* payload = malloc(payload_size);

    memcpy(payload, &byte0, sizeof(byte0));
    memcpy(payload + 1, &byte1, sizeof(byte1));

    if (extraPayloadlength == sizeof(uint16_t)) {
#if defined(_WIN32)
        uint16_t size_network_order = htons(size);
#else
        uint16_t size_network_order = htobe16(size);
#endif
        memcpy(payload + 2, &size_network_order, extraPayloadlength);
    } else if (extraPayloadlength == sizeof(uint64_t)) {
#if defined(_WIN32)
        uint64_t size_network_order = htonll(size);
#else
        uint64_t size_network_order = htobe64(size);
#endif
        memcpy(payload + 2, &size_network_order, extraPayloadlength);
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
        
        printf("payload (size): %zu\n", payload_size);

        printf("payload (masked): ");
        for (int i = 0; i < size; i++) {
            payload[2 + extraPayloadlength + sizeof(maskingkey) + i] = payload[2 + extraPayloadlength + sizeof(maskingkey) + i] ^ maskingkey[i % 4];
            printf("%X ", payload[2 + extraPayloadlength + sizeof(maskingkey) + i]);
        }
        printf("\n");
    }

    if (send(fd, payload, payload_size, 0) < 0) {
        fprintf(stderr, "send(): %s.\n", strerror(errno));
        free(payload);
        return FAILURE;
    }

    free(payload);

    return SUCCESS;
}

struct message websocket_recv(int fd) {
    bool FIN = false;

    struct message msg;
    memset(&msg, 0, sizeof(struct message));
    ((struct message_data*)msg.msgdata)->buffer = NULL;
    
    while (FIN != true) {
        uint8_t header[2];
        if (recv(fd, header, sizeof(header), MSG_WAITALL) <= 0) { // todo: make a test to figure out what recv returns (on linux it's ssize_t, on windows it's int. 4 bytes longer...)
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            msg.error = EXIT_FAILURE; return msg;
        }

        FIN = (header[0] & 0b10000000) != 0;
        printf("FIN: %s\n", FIN ? "True" : "False");

        if ((header[0] & 0b01110000) != 0) {
            fprintf(stderr, "RSV[1..3] has a non 0 value! Connection must be considered a FAIL!\n");
            msg.error = FAILURE; return msg;
        }

        enum opcodes opcode = header[0] & 0b00001111;
        printf("opcode: %i\n", opcode);

        bool masked = (header[1] & 0b10000000) != 0;
        printf("masked: %i\n", masked);
        if (masked == true) {
            fprintf(stderr, "Masked bit has a non 0 value! Connection must be considered a FAIL!\n");
            msg.error = FAILURE; return msg;
        }

        uint64_t payload_size = header[1] & 0b01111111;

        if (payload_size == 126) {
            if (recv(fd, (char*)&payload_size, sizeof(uint16_t), 0) <= 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                msg.error = FAILURE; return msg;
            }
            payload_size = htons(payload_size);
        }
        else if (payload_size == 127) {
            if (recv(fd, (char*)&payload_size, sizeof(uint64_t), 0) <= 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                msg.error = FAILURE; return msg;
            }
#if defined(_WIN32)
            payload_size = htonll(payload_size);
#else
            payload_size = be64toh(payload_size);
#endif
        }

        printf("payload size: %zu, current buffer size: %zu\n", payload_size, ((struct message_data*)msg.msgdata)->size);

        if (payload_size > 0) {
            if (((struct message_data*)msg.msgdata)->buffer == NULL) {
                ((struct message_data*)msg.msgdata)->buffer = malloc(payload_size);
            } else {
                printf("resizing buffer... %zu -> %zu\n", ((struct message_data*)msg.msgdata)->size, ((struct message_data*)msg.msgdata)->size + payload_size);
                resizebuffer(((struct message_data*)msg.msgdata)->buffer, ((struct message_data*)msg.msgdata)->size + payload_size);
            }
            
            if (recv(fd, (uint8_t*)(((struct message_data*)msg.msgdata)->buffer) + ((struct message_data*)msg.msgdata)->size, payload_size, MSG_WAITALL) <= 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                free(((struct message_data*)msg.msgdata)->buffer);
                msg.error = FAILURE; return msg;
            }

            printf("payload: ");
            for (int i = 0; i < payload_size; i++) {
                printf("%X ", *(uint8_t *)((uint8_t *)((struct message_data*)msg.msgdata)->buffer + ((struct message_data*)msg.msgdata)->size + i));
            }
            printf("\n");
        }

        ((struct message_data*)msg.msgdata)->size += payload_size;
        ((struct message_data*)msg.msgdata)->opcode = opcode;
    }

    // add the end string char.
    if (((struct message_data*)msg.msgdata)->opcode == TEXT) {
        resizebuffer(((struct message_data*)msg.msgdata)->buffer, ((struct message_data*)msg.msgdata)->size + 1);
        printf("resizing buffer... %zu -> %zu\n", ((struct message_data*)msg.msgdata)->size, ((struct message_data*)msg.msgdata)->size + 1);

        char endchar = '\0';
        memcpy((uint8_t*)(((struct message_data*)msg.msgdata)->buffer) + ((struct message_data*)msg.msgdata)->size, &endchar, 1);
    }

    return msg;
}
