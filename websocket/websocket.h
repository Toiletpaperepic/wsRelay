#include "parse_url.h"
#include "opcodes.h"
#include <stdbool.h>
#include <stdint.h>

struct message_data {
    uint64_t size;
    enum opcodes opcode;
    void* buffer;
};

struct message {
    bool error;
    uint8_t msgdata[sizeof(struct message_data)];
};

int websocket_send(int fd, void* buffer, uint64_t size, enum opcodes opcode, bool FIN);
struct message websocket_recv(int fd);
int websocket_connect(struct parsed_url purl);