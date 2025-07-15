#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "parse_url.h"

struct parsed_url parse_url(const char* url) {
    struct parsed_url purl;
    
    char protocol[strlen(url)];
    int protocol_size = 0;
    
    for (int i = 0; i < strlen(url); i++) {
        // printf("%c\n", url[i]);
        
        if (url[i] == ':') {
            break;
        } else {
            protocol[protocol_size] = url[i];
        }
        
        protocol_size++;
    }
    
    protocol[protocol_size] = '\0';
    // printf("Protocol: %s\n", protocol);

    assert(url[protocol_size] == ':' && url[protocol_size + 1] == '/' && url[protocol_size + 2] == '/');
    
    char address[strlen(url)];
    int address_size = 0;
    
    for (int i = protocol_size + 3; i < strlen(url); i++) {
        // printf("%c\n", url[i]);
        
        if (url[i] == ':' || url[i] == '/') {
            break;
        } else {
            address[address_size] = url[i];
        }
        
        address_size++;
    }
    
    address[address_size] = '\0';
    
    // printf("Address: %s\n", address);
    
    int port_size = 0;
    char port[strlen(url)];
    if (url[protocol_size + 3 + address_size] == ':') {
        for (int i = protocol_size + 3 + address_size + 1; i < strlen(url); i++) {
            // printf("%c\n", url[i]);
            
            if (url[i] == '/') {
                break;
            } else {
                port[port_size] = url[i];
            }
            
            port_size++;
        }
        
        port[port_size] = '\0';
        
        // printf("Port: %d\n", atoi(port));
    } else {
        // port = "8000";
    }
    
    char path[strlen(url)];
    int path_size = 0;
    
    for (int i = protocol_size + 3 + address_size + 1 + port_size; i < strlen(url); i++) {
        // printf("%c\n", url[i]);
        
        path[path_size] = url[i];
        path_size++;
    }
    
    path[path_size] = '\0';
    
    // printf("Path: %s\n", path);
    
    if (strcmp(protocol, "ws")) {
        purl.protocol = ws;
    } else if (strcmp(protocol, "wss")) {
        purl.protocol = wss;
    } else {
        purl.protocol = unknown;
    }
    
    purl.address = address;
    purl.port = atoi(port);
    purl.path = path;
    
    return purl;
}