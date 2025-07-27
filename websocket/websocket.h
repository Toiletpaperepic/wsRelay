#include "parse_url.h"

struct connection {
    struct parsed_url url;
    int fd;
};

struct connection websocket_connect(struct parsed_url purl);