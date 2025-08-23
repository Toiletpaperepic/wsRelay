#include "main.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int main() {
    printf("Starting local connection...\n");
    
    int socket = socket_bind(INADDR_ANY, 25565);
    int connection = socket_listen(socket);
    
    printf("Starting websocket connection...\n");
    struct connection ws_connection = websocket_connect(parse_url("ws://127.0.0.1:8000/connect"));
    // struct message msg = websocket_recv(connection);
    // printf("message from server %s\n", (char*)msg.buffer);
    // free(msg.buffer);
    
    uint8_t buffer[126] = {};
    while (strcmp((char *)buffer, "exit\n")) {
        memset(buffer, '\0', sizeof(buffer));

        int bytesrecv = recv(connection, buffer, sizeof(buffer), 0);
        if (bytesrecv < 0) {
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            exit(errno);
        }

        // printf("%i\n", bytesrecv);

        printf("Message from client: %s\n", (char *)buffer);
        websocket_send(ws_connection, buffer, bytesrecv);

        memset(buffer, '\0', sizeof(buffer));

        struct message msg = websocket_recv(ws_connection);

        if (send(connection, msg.buffer, msg.size, 0) < 0) {
            fprintf(stderr, "send(): %s.\n", strerror(errno));
            exit(errno);
        }
    }
    
    if (close(connection) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        exit(errno);
    }
    if (close(socket) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        exit(errno);
    }

    if (close(ws_connection.fd) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        exit(errno);
    }
    free_purl(ws_connection.url);

    return 0;
}
