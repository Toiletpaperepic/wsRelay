#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "main.h"

struct bridge {
    int con;
    struct connection wscon;
};

static volatile int sigint = 0;

void* inbound(void* arg) {
    uint8_t buffer[1024] = {};
    while (!sigint) {
        memset(buffer, '\0', sizeof(buffer));
        
        int bytesrecv = recv(((struct bridge*)arg)->con, buffer, sizeof(buffer), 0);
        if (bytesrecv < 0) {
            fprintf(stderr, "recv(): %s.\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        websocket_send(((struct bridge*)arg)->wscon, buffer, bytesrecv);
    }

    return NULL;
}

void* outbound(void* arg) {
    uint8_t buffer[1024] = {};
    while (!sigint) {
        memset(buffer, '\0', sizeof(buffer));
        struct message msg = websocket_recv(((struct bridge*)arg)->wscon);
        
        if (send(((struct bridge*)arg)->con, msg.buffer, msg.size, 0) < 0) {
            fprintf(stderr, "send(): %s.\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        free(msg.buffer);
    }

    return NULL;
}

int main() {
    printf("Starting local connection...\n");
    
    int socket = socket_bind(INADDR_ANY, 25565);
    int connection = socket_listen(socket);
    
    printf("Starting websocket connection...\n");
    struct connection ws_connection = websocket_connect(parse_url("ws://127.0.0.1:8000/connect"));
    
    struct bridge bridge;
    bridge.con = connection;
    bridge.wscon = ws_connection;

    pthread_t thread1;
    pthread_create(&thread1, NULL, inbound, (void*)&bridge);

    pthread_t thread2;
    pthread_create(&thread2, NULL, outbound, (void*)&bridge);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    if (close(connection) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (close(socket) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (close(ws_connection.fd) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    free((void*)ws_connection.url.address);
    free((void*)ws_connection.url.path);

    return 0;
}