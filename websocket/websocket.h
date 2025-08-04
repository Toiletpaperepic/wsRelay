#include "parse_url.h"
#include <stddef.h>

struct connection {
    struct parsed_url url;
    int fd;
};

void websocket_send(struct connection con, void* buffer, size_t size);
void websocket_recv(struct connection con, void* buffer, size_t size);
struct connection websocket_connect(struct parsed_url purl);