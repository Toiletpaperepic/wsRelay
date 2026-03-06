#include "parse_url.h"
#include "opcodes.h"
#include <stdbool.h>

struct message {
    uint64_t size;
    enum opcodes opcode;
    void* buffer;
};

int websocket_send(int fd, void* buffer, uint64_t size, enum opcodes opcode, bool FIN);
struct message websocket_recv(int fd);
int websocket_connect(struct parsed_url purl);