#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "parse_url.h"
#define STRING_TO_INT_CONVERSION 1
#include "common.h"

struct parsed_url parse_url(const char* url) {
    struct parsed_url purl;
    int cursor = 0;
    
    for (int i = 0; i < strlen(url); i++) {
        if (url[i] == ':') {
            char* protocol = malloc(i + 1);
            strncpy(protocol, url, i);
            protocol[i] = '\0';

            if (strcmp(protocol, "ws") == 0) {
                purl.protocol = ws;
            } else if (strcmp(protocol, "wss") == 0) {
                purl.protocol = wss;
            } else {
                purl.protocol = unknown;
            }

            free(protocol);

            cursor = i;
            break;
        }
    }
    
    // printf("Protocol: %u\n", purl.protocol);
    
    assert(url[cursor] == ':' && url[cursor + 1] == '/' && url[cursor + 2] == '/');
    cursor += 3;
    
    for (int i = cursor; i < strlen(url); i++) {
        // printf("%c\n", url[i]);
        
        if (url[i] == ':' || url[i] == '/') {
            purl.address = malloc(i - cursor + 1);
            strncpy(purl.address, url + cursor, i - cursor);
            purl.address[i - cursor] = '\0';

            cursor = i;
            break;
        } 
    }
    
    // printf("Address: %s\n", purl.address);
    
    if (url[cursor] == ':') {
        cursor += 1;

        for (int i = cursor; i < strlen(url); i++) {
            if (url[i] == '/') {
                char* string_port = malloc(i - cursor + 1);
                strncpy(string_port, url + cursor, i - cursor);
                string_port[i - cursor] = '\0';

                purl.port = strtouint16(string_port); // fixme: this function has been replaced, but there isn't any error handling.
                
                free(string_port);

                cursor = i;
                break;
            } 
        }
    } else {
        if (purl.protocol == wss) {
            purl.port = 443;
        } else {
            purl.port = 80;
        }
    }

    // printf("Port: %d\n", purl.port);

    purl.path = malloc(strlen(url) - cursor + 1);
    strncpy(purl.path, url + cursor, strlen(url) - cursor + 1);
    purl.path[strlen(url) - cursor] = '\0';
    // cursor = strlen(url) - cursor + 1;

    // printf("Path: %s\n", purl.path);
    
    return purl;
}