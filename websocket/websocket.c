#include <base64-cstring.h>
#include <sys/random.h>
#include <arpa/inet.h>
#include <assert.h>
#include "websocket.h"
#include "check.h"

// https://en.wikipedia.org/wiki/WebSocket#Protocol

void make_http_header(struct connection con, char* message) {
    strcat(message, "GET ");
    strcat(message, con.url.path);
    strcat(message, " HTTP/1.1\n");

    // Host:
    strcat(message, "Host: ");
    strcat(message, con.url.address);
    strcat(message, "\n");

    // User Agent:
    strcat(message, "User-Agent: wsp-websocket/1.0\n");

    // Accept:
    strcat(message, "Accept: */*\n");

    // Upgrade: 
    strcat(message, "Upgrade: websocket\n");

    // Connection:
    strcat(message, "Connection: Upgrade\n");

    // WebSocket Version: 
    strcat(message, "Sec-WebSocket-Version: 13\n");

    char nonce[16 + 1];
    getrandom(&nonce, sizeof(nonce), 0);

    for (int i = 0; i < sizeof(nonce); i++) {
        while (nonce[i] == '\0') {
            getrandom(&nonce[i], sizeof(char), 0);
        }
    }

    nonce[sizeof(nonce) - 1] = '\0';
    const char* key = base64_encode_c((const char*)&nonce);

    assert(strlen(key) == 24);
    
    // WebSocket Key: 
    strcat(message, "Sec-WebSocket-Key: ");
    strcat(message, key);
    strcat(message, "\n");
    
    free((void*)key);

    // Blank Line (end of request)
    strcat(message, "\n");
}

struct connection websocket_connect(struct parsed_url purl) {
    struct connection con;
    con.url = purl;
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    check2(fd < 0);
    con.fd = fd;
    
    struct in_addr addr;
    check2(inet_aton(con.url.address, &addr) < 0);
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(con.url.port);
    serverAddress.sin_addr.s_addr = addr.s_addr;
    
    check2(connect(fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0);

    // tell the server to upgrade the connection 
    char message[1024] = ""; 
    make_http_header(con, message);

    printf("Sending message: %s\n", message);
    websocket_send(con, message, sizeof(message));

    char buffer[1024] = {};
    recv(con.fd, buffer, sizeof(buffer), 0);
    printf("received accept message: %s\n", buffer);
    memset(buffer, '\0', sizeof(buffer));

    //TODO: check for a valid response?

    return con;
}

void websocket_send(struct connection con, void* buffer, size_t size) {
    check(send(con.fd, buffer, size, 0) < 0);
}

struct message websocket_recv(struct connection con) {
    char header[2] = {};
    recv(con.fd, header, sizeof(header), 0);

    int FIN = (header[0] & 0b10000000) != 0;
    printf("FIN: %i\n", FIN);
    assert(FIN == 1);

    int opcode = header[0] & 0b00001111;
    printf("opcode: %i\n", opcode);
    assert(opcode == 1 || opcode == 2);

    int masked = (header[1] & 0b10000000) != 0;
    printf("masked: %i\n", masked);
    assert(masked == 0);

    unsigned int payload_size = header[1] & 0b01111111;
    printf("payload size: %i\n", payload_size);

    memset(header, '\0', sizeof(header));

    struct message msg;
    msg.size = payload_size;
    msg.buffer = malloc(payload_size);
    
    recv(con.fd, msg.buffer, payload_size, 0);

    return msg;
}