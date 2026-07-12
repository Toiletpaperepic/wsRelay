#ifndef WEBSOCKET_HTTP_H
#define WEBSOCKET_HTTP_H

#include "dll_export.h"
#include <stdint.h>

struct http_header {
    const char* header_name;
    const char* header_content;
};

struct http_response_read_result_successful {
    const char* httpversion;
    uint16_t httpcode; // we could change this into a int but a string is fine.
    unsigned int headerslist_len;
    struct http_header* headerslist;
    char buffer[1024];
};

struct http_response_read_result {
    int error;
    uint8_t data[sizeof(struct http_response_read_result_successful)];
};

struct http_header* getheaderfromlist(const char* name, unsigned int headerslist_len, struct http_header* headerslist);
DLL_EXPORT struct http_response_read_result read_http_response_header(int fd);
DLL_EXPORT void make_user_agent(char** destinationstring);
const char* make_http_header(struct parsed_url purl);
#endif
