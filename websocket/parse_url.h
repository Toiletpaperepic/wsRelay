#include "dll_export.h"
#include <stdint.h>

enum protocol_type {
    unknown,
    ws,
    wss,
};

struct parsed_url {
    enum protocol_type protocol;
    char* address;
    uint16_t port;
    char* path;
};

DLL_EXPORT struct parsed_url parse_url(const char* url);