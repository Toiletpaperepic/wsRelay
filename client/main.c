#include "main.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    // printf("Starting local connection...\n");

    // int socket = socket_bind(INADDR_ANY, 25565);
    // int connection = socket_listen(socket);

    // void *buffer[1024] = {};
    // while (strcmp((char *)buffer, "exit\n")) {
    //     memset(buffer, '\0', sizeof(buffer));
    //     check(recv(connection, buffer, sizeof(buffer), 0) < 0);
    //     printf("%s: Message from client: %s\n", argv[0], (char *)buffer);
    // }
    
    // check(close(connection) < 0);
    // check(close(socket) < 0);

    printf("Starting websocket connection...\n");

    struct connection connection = websocket_connect(parse_url("ws://127.0.0.1:8000/connect"));

    // struct message msg = websocket_recv(connection);
    // printf("message from server %s\n", (char*)msg.buffer);
    // free(msg.buffer);
    
    char message[14] = "Hello, World!\n";
    while(1) {
        websocket_send(connection, &message, sizeof(message));
        sleep(5);
    }

    close(connection.fd);
    free_purl(connection.url);

    return 0;
}
