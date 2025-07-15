#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "websocket.h"
#include "check.h"
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

    char message[1024] = ""; 
    make_http_header(&url, message);

    printf("Sending message: %s\n", message);
    send(connection, message, strlen(message), 0);

    char buffer[1024] = {};
    recv(connection, buffer, sizeof(buffer), 0);
    printf("%s: message from server %s\n", argv[0], buffer);
    memset(buffer, '\0', sizeof(buffer));

    recv(connection, buffer, sizeof(buffer), 0);
    printf("%s: message from server %s\n", argv[0], buffer);
    memset(buffer, '\0', sizeof(buffer));

    close(connection);

    return 0;
}