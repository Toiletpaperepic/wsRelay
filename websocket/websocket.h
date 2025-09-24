#include "parse_url.h"
#include "opcodes.h"
#include <stddef.h>

struct connection {
    struct parsed_url url;
    int fd;
};

struct message {
    unsigned int size;
    enum opcodes opcodes;
    void* buffer;
};

void websocket_send(struct connection con, void* buffer, size_t size);
struct message websocket_recv(struct connection con);
struct connection websocket_connect(struct parsed_url purl);