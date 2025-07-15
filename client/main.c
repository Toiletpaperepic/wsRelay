#include "main.h"

int main(int argc, char* argv[]) {
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

    struct parsed_url url = parse_url("ws://localhost:8000/connect");
    int connection = websocket_connect(&url);

    char buffer[1024] = {};
    check(recv(connection, buffer, sizeof(buffer), 0) < 0);
    printf("%s: message from server %s\n", argv[0], buffer);
    memset(buffer, '\0', sizeof(buffer));

    check(recv(connection, buffer, sizeof(buffer), 0) < 0);
    printf("%s: message from server %s\n", argv[0], buffer);
    memset(buffer, '\0', sizeof(buffer));

    close(connection);

    return 0;
}