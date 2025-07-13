#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "check.h"
#include "main.h"
#include "websocket.h"

int main(int argc, char* argv[]) {
    printf("Starting local connection...\n");

    int socket = socket_bind(INADDR_ANY, 25565);
    int connection = socket_listen(socket);

    void *buffer[1024] = {};
    while (strcmp((char *)buffer, "exit\n")) {
        memset(buffer, '\0', sizeof(buffer));
        check(recv(connection, buffer, sizeof(buffer), 0) < 0);
        printf("%s: Message from client: %s\n", argv[0], (char *)buffer);
    }
    
    check(close(connection) < 0);
    check(close(socket) < 0);

    printf("Starting websocket connection...\n");

    struct parsed_url url = parse_url("ws://localhost:8000/connect");
    int connection = websocket_connect(url);

    char message[1024] = ""; 
    strcat(message, "GET ");
    strcat(message, url.path);
    strcat(message, " HTTP/1.1\n");

    // Host:
    strcat(message, "Host: ");
    strcat(message, url.address);
    strcat(message, "\n");\

    // User Agent:
    strcat(message, "User-Agent: wsp-websocket/1.0\n");

    // Accept:
    strcat(message, "Accept: */*\n");

    // strcat(message, "Upgrade: websocket\n");

    // Connection:
    // strcat(message, "Connection: Upgrade\n");

    // strcat(message, "Sec-WebSocket-Protocol: stream\n");

    // Blank Line (end of request)
    strcat(message, "\n");

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