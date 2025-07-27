#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "parse_url.h"

struct parsed_url parse_url(const char* url) {
    struct parsed_url purl;
    int cursor = 0;
    
    for (int i = 0; i < strlen(url); i++) {
        // printf("%c\n", url[i]);
        
        if (url[i] == ':') {
            char protocol[i + 1];
            strncpy(protocol, url, i);
            protocol[i] = '\0';

            if (strcmp(protocol, "ws") == 0) {
                purl.protocol = ws;
            } else if (strcmp(protocol, "wss") == 0) {
                purl.protocol = wss;
            } else {
                purl.protocol = unknown;
            }

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
            char address[i - cursor + 1];
            strncpy(address, url + cursor, i - cursor);
            address[i - cursor] = '\0';

            purl.address = strdup(address);

            cursor = i;
            break;
        } 
    }
    
    // printf("Address: %s\n", purl.address);
    
    if (url[cursor] == ':') {
        cursor += 1;

        for (int i = cursor; i < strlen(url); i++) {
            // printf("%c\n", url[i]);
        
            if (url[i] == '/') {
                char port[i - cursor + 1];
                strncpy(port, url + cursor, i - cursor);
                port[i - cursor] = '\0';

                purl.port = atoi(port);

                cursor = i;
                break;
            } 
        }
    } else {
        purl.port = 8000;
    }

    // printf("Port: %d\n", purl.port);

    char path[strlen(url) - cursor + 1];
    strncpy(path, url + cursor, strlen(url) - cursor + 1);
    path[strlen(url) - cursor] = '\0';
    purl.path = strdup(path);
    // cursor = strlen(url) - cursor + 1;

    // printf("Path: %s\n", path);
    
    return purl;
}