#if defined(_WIN32)
#include <windows.h>
// #include <stdio.h>
#else 
#if defined(__ANDROID__) && __ANDROID_API__ < 28
#include <sys/syscall.h>
#define getrandom(buf,buflen,flags) syscall(SYS_getrandom,buf,buflen,flags)
#else
#include <sys/random.h>
#endif
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <stdbool.h>
#include <base64.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include "wsrelay.h"
#define RESIZEBUFFER_CUSTOM_ERROR 1
#include "common.h"

void appendchar(char** destinationstring, const char* sourcestring) { 
    resizebuffer(*destinationstring, strlen(*destinationstring) + strlen(sourcestring) + 1, , true); 
    strcat(*destinationstring, sourcestring);
} 

/// going for something like this -> "wsrRelay/v1.0 (Linux; gcc 15.2.1; x86_64; +https://github.com/Toiletpaperepic/wsRelay/) Hostname/Apollo-Lake"
void make_user_agent(char** destinationstring) {
    appendchar(destinationstring, "User-Agent: wsrRelay/");
    appendchar(destinationstring, __PROJECT_VERSION__);
    appendchar(destinationstring, " ("); 
    
    // TODO: create USER_AGENT_LESS_INFO env
    appendchar(destinationstring, OS_NAME);
    
    appendchar(destinationstring, "; ");
    
    appendchar(destinationstring, COMPILER_NAME);
    appendchar(destinationstring, " ");
    appendchar(destinationstring, COMPILER_VERSION);
    
    appendchar(destinationstring, "; ");
    
    appendchar(destinationstring, PROCESSOR_ARCHITECTURE);
    
    appendchar(destinationstring, "; +https://github.com/Toiletpaperepic/wsRelay/) ");

#if defined(_WIN32)
    char hostname[256];
#else
    char hostname[HOST_NAME_MAX];
#endif
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        fprintf(stderr, "gethostname(): %s.\n", strerror(errno));
    } else {
        appendchar(destinationstring, "Hostname/");
        appendchar(destinationstring, hostname);
    }
    
    appendchar(destinationstring, "\r\n");
}

const char* make_http_header(struct parsed_url purl) {
    char* message = malloc(1);
    message[0] = '\0';

    appendchar(&message, "GET ");
    appendchar(&message, purl.path);
    appendchar(&message, " HTTP/1.1\r\n");

    // Host:
    appendchar(&message, "Host: ");
    appendchar(&message, purl.address);
    appendchar(&message, "\r\n");

    // User Agent:
    make_user_agent(&message);

    // Accept:
    appendchar(&message, "Accept: */*\r\n");

    // Upgrade: 
    appendchar(&message, "Upgrade: websocket\r\n");

    // Connection:
    appendchar(&message, "Connection: Upgrade\r\n");

    // WebSocket Version: 
    appendchar(&message, "Sec-WebSocket-Version: 13\r\n");

    uint8_t nonce[16];
#if defined(_WIN32)
    BCRYPT_ALG_HANDLE handle;
    NTSTATUS error;

    error = BCryptOpenAlgorithmProvider(&handle, BCRYPT_RNG_ALGORITHM,NULL,0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptOpenAlgorithmProvider(): %lX.\n", error);
        exit(EXIT_FAILURE);
    }
    
    error = BCryptGenRandom(handle, (PUCHAR)nonce, sizeof(nonce), 0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptGenRandom(): %lX.\n", error);
        exit(EXIT_FAILURE);
    }
    
    error = BCryptCloseAlgorithmProvider(handle,0);
    if (!BCRYPT_SUCCESS(error)) {
        fprintf(stderr, "BCryptCloseAlgorithmProvider(): %lX.\n", error);
        exit(EXIT_FAILURE);
    }
#else
    getrandom(&nonce, sizeof(nonce), 0);
#endif
    const char* key = base64_encode_no_lf(&nonce, sizeof(nonce), NULL);

    assert(strlen(key) == 24);
    
    // WebSocket Key: 
    appendchar(&message, "Sec-WebSocket-Key: ");
    appendchar(&message, key);
    appendchar(&message, "\r\n");

    free((void*)key);

    // Blank Line (end of request)
    appendchar(&message, "\r\n");

    return message;
}

static void strgotocharuntil(char** strptr, char character) {
    while(strlen(*strptr) != 0) {
        if (**strptr == character) {
            break;
        }
        (*strptr)++; 
    }
}

struct http_response_read_result read_http_response_header(int fd) {
    // char buffer[1024];
    struct http_response_read_result rrr;
    memset(&rrr, 0, sizeof(struct http_response_read_result));
    
    // part 1: read response until end of request (\r\n\r\n)
    {
        unsigned int cursor = 4;

        if (recv(fd, ((struct http_response_read_result_success*)rrr.data)->buffer, cursor, MSG_WAITALL) < 0) {
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            close(fd);
            rrr.error = FAILURE; return rrr;
        }
        
        // recvive until we reach \r\n\r\n
        for (; !(((struct http_response_read_result_success*)rrr.data)->buffer[cursor - 4] == '\r' && ((struct http_response_read_result_success*)rrr.data)->buffer[cursor - 3] == '\n' && ((struct http_response_read_result_success*)rrr.data)->buffer[cursor - 2] == '\r' && ((struct http_response_read_result_success*)rrr.data)->buffer[cursor - 1] == '\n'); cursor++) {
            if (cursor > sizeof(((struct http_response_read_result_success*)rrr.data)->buffer)) {
                fprintf(stderr, "Respose buffer is full! Considering read as a FAILURE!\n");
                rrr.error = FAILURE; return rrr;
            }
    
            if (recv(fd, ((struct http_response_read_result_success*)rrr.data)->buffer + cursor, 1, MSG_WAITALL) < 0) {
                fprintf(stderr, "recv(): %s.\n", strerror(errno));
                close(fd);
                rrr.error = FAILURE; return rrr;
            }
        }
    
        ((struct http_response_read_result_success*)rrr.data)->buffer[cursor] = '\0';
        unsigned int response_size = cursor - 4;
    
        printf("received accept message with size of %i, %s\n", response_size - 1, ((struct http_response_read_result_success*)rrr.data)->buffer);
    }

    ((struct http_response_read_result_success*)rrr.data)->headerslist_len = 1;
    char** headerslist = malloc(((struct http_response_read_result_success*)rrr.data)->headerslist_len * sizeof(* headerslist));

    // part 2: split headers
    {
        char *ch;
        ch = strtok(((struct http_response_read_result_success*)rrr.data)->buffer, "\r\n");
        while (ch != NULL) {
            headerslist[((struct http_response_read_result_success*)rrr.data)->headerslist_len - 1] = ch;
            ((struct http_response_read_result_success*)rrr.data)->headerslist_len++;

            resizebuffer(headerslist, ((struct http_response_read_result_success*)rrr.data)->headerslist_len * sizeof(* headerslist), free(headerslist); rrr.error = FAILURE; return rrr;, true);

            ch = strtok(NULL, "\r\n");
        }
    }

    // part 3: now check if we got a good response on the first header.
    {
        char* firstheader = headerslist[0];

        strgotocharuntil(&firstheader, '/');
        firstheader++;
        ((struct http_response_read_result_success*)rrr.data)->httpversion = firstheader;
        
        strgotocharuntil(&firstheader, ' ');
        *firstheader = '\0'; firstheader++;
        
        // printf("HTTP version: %s\n", httpversion);

        ((struct http_response_read_result_success*)rrr.data)->httpcode = firstheader;
        strgotocharuntil(&firstheader, ' ');
        *firstheader = '\0'; firstheader++;

        // printf("HTTP code: %s\n", httpcode);
    }

    ((struct http_response_read_result_success*)rrr.data)->headerslist = malloc(((struct http_response_read_result_success*)rrr.data)->headerslist_len * sizeof(* ((struct http_response_read_result_success*)rrr.data)->headerslist));
    
    // part 4: split headerslist into two parts (name, content).
    {
        for (int i = 1; i < ((struct http_response_read_result_success*)rrr.data)->headerslist_len - 1; i++) {
            char* header_start = headerslist[i];
            char* header = header_start;

            strgotocharuntil(&header, ':');
            *header = '\0';
            if (*(header + 1) == ' ') { // if a space is found, move forward.
                header++;
            }
            header++;
            
            ((struct http_response_read_result_success*)rrr.data)->headerslist[i].header_name = header_start;
            ((struct http_response_read_result_success*)rrr.data)->headerslist[i].header_content = header;

            // printf("header name: %s\n", ((struct http_response_read_result_success*)rrr.data)->headerslist[i].header_name);
            // printf("header content: %s\n", ((struct http_response_read_result_success*)rrr.data)->headerslist[i].header_content);
        }
    }

    free(headerslist);

    return rrr;
}

struct http_header* getheaderfromlist(const char* name, unsigned int headerslist_len, struct http_header* headerslist) {
    for (int i = 1; i < headerslist_len - 1; i++) {
        if (strcmp(name, headerslist[i].header_name) == 0) {
            return &headerslist[i];
        }
    }

    return NULL;
} 