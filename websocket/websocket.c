#include "common.h"
#include PLATFORM_NETWORK_HEADER

#if defined(_WIN32)
#include <windows.h>
#include <ws2tcpip.h>
#else
#if defined(__ANDROID__) && __ANDROID_API__ < 28
#include <sys/syscall.h>
#define getrandom(buf,buflen,flags) syscall(SYS_getrandom,buf,buflen,flags)
#else
#include <sys/random.h>
#endif
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
#include "wsrelay.h"

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
            error("getaddrinfo: %s", strerror(errno));
        } else {
            error("getaddrinfo: gai_strerror: %s", gai_strerror(error));
        }
#else
        error("getaddrinfo: %i", WSAGetLastError());
#endif
        return -1;
    }

    bool success = false;
    
    // loop over all returned results
    for (ai = result; ai != NULL && !success; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd == PLATFORM_NETWORK_REP_INVALID_SOCKET) {
            error("socket(): " PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE, PLATFORM_NETWORK_GET_ERROR);
            continue;
        }

        if (ai->ai_family == AF_INET) {
            debug("Using ipv4.");
            //trace("\tIPv4 address %s\n", inet_ntoa(((struct sockaddr_in*)(struct sockaddr_in*)ai->ai_addr)->sin_addr));
            ((struct sockaddr_in*)ai->ai_addr)->sin_port = htons(purl.port);
        } else if (ai->ai_family == AF_INET6) {
            debug("Using ipv6.");
            ((struct sockaddr_in6*)ai->ai_addr)->sin6_port = htons(purl.port);
        }
        
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == PLATFORM_NETWORK_REP_SOCKET_ERROR) {
            error("connect(): " PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE, PLATFORM_NETWORK_GET_ERROR);
            continue;
        }

        success = !success;
    }

    freeaddrinfo(result);

    if (!success) {
        return NEGFAILURE;
        error("Failed to connected to a server!\n");
    } else {
        debug("Successfuly connected to a server!\n");
    }

    // tell the server to upgrade the connection 
    const char* message = make_http_header(purl);
    debug("Sending message: %s", message);

    if (send(fd, message, strlen(message), 0) == PLATFORM_NETWORK_REP_SOCKET_ERROR) {
        error("send(): " PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE, PLATFORM_NETWORK_GET_ERROR);
        free((void*)message);
#if HAVE_WINSOCK2_H
        closesocket(fd);
#elif HAVE_SYS_SOCKET_H
        close(fd);
#endif
        return -1;
    }

    free((void*)message);

    struct http_response_read_result rrr = read_http_response_header(fd);
    if (rrr.error == FAILURE) 
        return NEGFAILURE;

    struct http_response_read_result_successful rrrs = *(struct http_response_read_result_successful*)rrr.data;
    // debug("%s %s", getheaderfromlist("Connection", rrrs.headerslist_len, rrrs.headerslist)->header_name, getheaderfromlist("Connection", rrrs.headerslist_len, rrrs.headerslist)->header_content);

    if (rrrs.httpcode != 101) {
        error("Server did not send a 101 response! (got %hu)", rrrs.httpcode);
        free(rrrs.headerslist); return NEGFAILURE;
    }

    if (getheaderfromlist("Connection", rrrs.headerslist_len, rrrs.headerslist) == NULL) {
        error("Server did not send a Connection header!");
        free(rrrs.headerslist); return NEGFAILURE;
    }

    if (getheaderfromlist("Upgrade", rrrs.headerslist_len, rrrs.headerslist) == NULL) {
        error("Server did not send a Upgrade header!");
        free(rrrs.headerslist); return NEGFAILURE;
    }

    debug("Server sent a good response!");

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
    unsigned int extra_payload_size_length = 0;

    if (size <= 125) { // size fits in 7 bits
        byte1 = byte1 | (uint8_t)size;
        trace("size is smaller then 125");
    } else if (size >= 125 && size < UINT16_MAX) { // size fits in 16 bits
        byte1 = byte1 | 126;
        extra_payload_size_length = sizeof(uint16_t);
        trace("size is smaller then UINT16_MAX");
    } else if (size >= 125 && size > UINT16_MAX && size < UINT64_MAX) { // size fits in 64 bits
        byte1 = byte1 | 127;
        extra_payload_size_length = sizeof(uint64_t);
        trace("size is smaller then UINT64_MAX");
    }

    uint8_t maskingkey[4];
#if defined(_WIN32)
    BCRYPT_ALG_HANDLE handle;
    NTSTATUS error;

    error = BCryptOpenAlgorithmProvider(&handle, BCRYPT_RNG_ALGORITHM,NULL,0);
    if (!BCRYPT_SUCCESS(error)) {
        trace("BCryptOpenAlgorithmProvider(): %lX.", error);
        return FAILURE;
    }
    
    error = BCryptGenRandom(handle, (unsigned char*)maskingkey, sizeof(maskingkey), 0);
    if (!BCRYPT_SUCCESS(error)) {
        trace("BCryptGenRandom(): %lX.", error);
        return FAILURE;
    }
    
    error = BCryptCloseAlgorithmProvider(handle,0);
    if (!BCRYPT_SUCCESS(error)) {
        trace("BCryptCloseAlgorithmProvider(): %lX.", error);
        return FAILURE;
    }
#else
    getrandom(&maskingkey, sizeof(maskingkey), 0);
#endif

#if LOGGER_COMPILE_OUT != 1 
    if(getalt()->trace) {
        fprintf(stderr, MAG "[trace] (%s) " RESET "masking key: ", __func__);
        for (int i = 0; i < sizeof(maskingkey); i++)
            fprintf(stderr, "%X ", maskingkey[i]);
        fprintf(stderr, "\n");
    };
#endif

    size_t header_size = 2 + extra_payload_size_length + sizeof(maskingkey);
    size_t payload_size = header_size + size;
    uint8_t* payload = malloc(payload_size);

    memcpy(payload, &byte0, sizeof(byte0));
    memcpy(payload + 1, &byte1, sizeof(byte1));

    if (extra_payload_size_length == sizeof(uint16_t)) {
#if defined(_WIN32)
        uint16_t size_network_order = htons(size);
#else
        uint16_t size_network_order = htobe16(size);
#endif
        memcpy(payload + 2, &size_network_order, extra_payload_size_length);
    } else if (extra_payload_size_length == sizeof(uint64_t)) {
#if defined(_WIN32)
        uint64_t size_network_order = htonll(size);
#else
        uint64_t size_network_order = htobe64(size);
#endif
        memcpy(payload + 2, &size_network_order, extra_payload_size_length);
    }

    memcpy(payload + 2 + extra_payload_size_length, maskingkey, sizeof(maskingkey));

    if (buffer == NULL && size == 0) {
        trace("no payload provided.");
    } else {
        memcpy(payload + header_size, buffer, size);

#if LOGGER_COMPILE_OUT != 1 
        if(getalt()->trace) {
            fprintf(stderr, MAG "[trace] (%s) " RESET "payload: ", __func__);
            for (int i = 0; i < size; i++) {
                fprintf(stderr, "%X ", payload[header_size + i]);
            }
            fprintf(stderr, "\n");
        }
#endif
        
        trace("payload (size): %zu", payload_size);

        for (int i = 0; i < size; i++) {
            /* 
            From perf, this is the slowest operation of the entire codebase,
            unfortunately i dont think theres anything i can do to fix this since this is required.
            
            running a speed test show 1711 download, and 305 upload (6/20/26). Not good over localhost :(
            55.44%  wsrelay  libwsrelay-websocket.so.0.0.2  [.] websocket_send    
            22.59 │160:   xor    %dl,-0x3(%r9,%rcx,1)
            20.28 │       xor    %sil,-0x2(%r9,%rcx,1)
            22.76 │       xor    %dil,-0x1(%r9,%rcx,1)
            22.81 │       xor    %r8b,(%r9,%rcx,1)
            */
            payload[header_size + i] = payload[header_size + i] ^ maskingkey[i % 4];
        }

#if LOGGER_COMPILE_OUT != 1 
        if(getalt()->trace) {
            fprintf(stderr, MAG "[trace] (%s) " RESET "payload (masked): ", __func__);
            for (int i = 0; i < size; i++) {
                fprintf(stderr, "%X ", payload[header_size + i]);
            }
            fprintf(stderr,"\n");
        }
#endif
    }

    if (send(fd, payload, payload_size, 0) == PLATFORM_NETWORK_REP_SOCKET_ERROR) {
        error("send(): " PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE, PLATFORM_NETWORK_GET_ERROR);
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
        if (recv(fd, header, sizeof(header), MSG_WAITALL) == PLATFORM_NETWORK_REP_SOCKET_ERROR) { // todo: make a test to figure out what recv returns (on linux it's ssize_t, on windows it's int. 4 bytes longer...)
            error("recv(): " PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE, PLATFORM_NETWORK_GET_ERROR);
            msg.error = EXIT_FAILURE; return msg;
        }

        FIN = (header[0] & 0b10000000) != 0;
        trace("FIN: %s", FIN ? "True" : "False");

        if ((header[0] & 0b01110000) != 0) {
            error("RSV[1..3] has a non 0 value! Connection must be considered a FAIL!");
            msg.error = FAILURE; return msg;
        }

        enum opcodes opcode = header[0] & 0b00001111;
        trace("opcode: %i", opcode);

        bool masked = (header[1] & 0b10000000) != 0;
        trace("masked: %i", masked);
        if (masked == true) {
            error("Masked bit has a non 0 value! Connection must be considered a FAIL!\n");
            msg.error = FAILURE; return msg;
        }

        uint64_t payload_size = header[1] & 0b01111111;

        if (payload_size == 126) {
            if (recv(fd, (char*)&payload_size, sizeof(uint16_t), 0) == PLATFORM_NETWORK_REP_SOCKET_ERROR) {
                error("recv(): " PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE, PLATFORM_NETWORK_GET_ERROR);
                msg.error = FAILURE; return msg;
            }
            payload_size = htons(payload_size);
        }
        else if (payload_size == 127) {
            if (recv(fd, (char*)&payload_size, sizeof(uint64_t), 0) == PLATFORM_NETWORK_REP_SOCKET_ERROR) {
                error("recv(): " PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE, PLATFORM_NETWORK_GET_ERROR);
                msg.error = FAILURE; return msg;
            }
#if defined(_WIN32)
            payload_size = htonll(payload_size);
#else
            payload_size = be64toh(payload_size);
#endif
        }

        trace("payload size: %zu, current buffer size: %zu", payload_size, ((struct message_data*)msg.msgdata)->size);

        if (payload_size > 0) {
            if (((struct message_data*)msg.msgdata)->buffer == NULL) {
                ((struct message_data*)msg.msgdata)->buffer = malloc(payload_size);
            } else {
                trace("resizing buffer... %zu -> %zu", ((struct message_data*)msg.msgdata)->size, ((struct message_data*)msg.msgdata)->size + payload_size);
                resizebuffer(((struct message_data*)msg.msgdata)->buffer, ((struct message_data*)msg.msgdata)->size + payload_size);
            }
            
            if (recv(fd, (uint8_t*)(((struct message_data*)msg.msgdata)->buffer) + ((struct message_data*)msg.msgdata)->size, payload_size, MSG_WAITALL) == PLATFORM_NETWORK_REP_SOCKET_ERROR) {
                trace("recv(): " PLATFORM_NETWORK_GET_ERROR_PRINT_TYPE, PLATFORM_NETWORK_GET_ERROR);
                free(((struct message_data*)msg.msgdata)->buffer);
                msg.error = FAILURE; return msg;
            }

#if LOGGER_COMPILE_OUT != 1 
            if(getalt()->trace) {
                fprintf(stderr, MAG "[trace] (%s) " RESET "payload: ", __func__);
                for (int i = 0; i < payload_size; i++) {
                    fprintf(stderr, "%X ", *(uint8_t *)((uint8_t *)((struct message_data*)msg.msgdata)->buffer + ((struct message_data*)msg.msgdata)->size + i));
                }
                fprintf(stderr, "\n");
            }
#endif

        }

        ((struct message_data*)msg.msgdata)->size += payload_size;
        ((struct message_data*)msg.msgdata)->opcode = opcode;
    }

    // add the end string char.
    if (((struct message_data*)msg.msgdata)->opcode == TEXT) {
        resizebuffer(((struct message_data*)msg.msgdata)->buffer, ((struct message_data*)msg.msgdata)->size + 1);
        trace("resizing buffer... %zu -> %zu", ((struct message_data*)msg.msgdata)->size, ((struct message_data*)msg.msgdata)->size + 1);

        char endchar = '\0';
        memcpy((uint8_t*)(((struct message_data*)msg.msgdata)->buffer) + ((struct message_data*)msg.msgdata)->size, &endchar, 1);
    }

    return msg;
}
