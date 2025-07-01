#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include "socket.h"
#include "websocket.h"

int socket_bind(in_addr_t address, uint16_t port) {
    int startPointSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = address;

    int error = bind(startPointSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); 
    if (error != 0) {
        printf("Socket failed to bind! exiting...\n");
        exit(1);
    }
    
    return startPointSocket;
}

int socket_listen(int startPointSocket) {
    listen(startPointSocket, 1);
    int clientSocket = accept(startPointSocket, nullptr, nullptr);
    return clientSocket;
}

void socket_close(int startPointSocket) {
    close(startPointSocket);
}
