#include "parse_url.h"
#include "opcodes.h"
#include <stddef.h>
#include <stdint.h>

struct message {
    uint64_t size;
    enum opcodes opcode;
    void* buffer;
};

void websocket_send(int fd, void* buffer, uint64_t size, enum opcodes opcode, bool FIN);
struct message websocket_recv(int fd);
int websocket_connect(struct parsed_url purl);