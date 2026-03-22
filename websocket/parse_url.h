#include "dll_export.h"
#include <stdint.h>

enum protocol_type {
    unknown,
    ws,
    wss,
};

struct parsed_url {
    enum protocol_type protocol;
    const char* address;
    uint16_t port;
    const char* path;
};

DLL_EXPORT struct parsed_url parse_url(const char* url);