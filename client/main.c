#include <assert.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "main.h"

struct route_c {
    int con;
    struct connection wscon;
};

#define ROUTE_LOCAL_SOCKET 0
#define ROUTE_WEBSOCKET 1

void* route(void* arg) {
    printf("Route thread started!\n");

    int epollfd = epoll_create1(0);
    if (epollfd < 0) {
        fprintf(stderr, "epoll_create1(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    // add file descriptor to the queue
    struct epoll_event epolleventlocalsocket;
    // epolleventlocalsocket.data.fd = ((struct route_c*)arg)->con;
    epolleventlocalsocket.data.u32 = ROUTE_LOCAL_SOCKET;
    epolleventlocalsocket.events = EPOLLIN;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ((struct route_c*)arg)->con, &epolleventlocalsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    struct epoll_event epolleventwebsocket;
    // epolleventwebsocket.data.ptr = &(((struct route_c*)arg)->wscon);
    epolleventwebsocket.data.u32 = ROUTE_WEBSOCKET;
    epolleventwebsocket.events = EPOLLIN;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ((struct route_c*)arg)->wscon.fd, &epolleventwebsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    bool loop = true;
    while (loop) {
        struct epoll_event epe[5];
        
        printf("waiting for packets...\n");
        int fdevents = epoll_wait(epollfd, epe, sizeof(epe), 1000 * 5);
        // printf("packets recived.\n");
        if (fdevents < 0)  {
            fprintf(stderr, "epoll_wait(): %s.\n", strerror(errno));
            goto FAILURE;
        } else if (fdevents == 0) {
            // not ready
            // printf("epoll not ready!\n");
        } else {
            for (int i = 0; i < fdevents; i++) {
                uint8_t buffer[65535] = {};

                if (epe[i].data.u32 == ROUTE_LOCAL_SOCKET) {
                    int bytesrecv = recv(((struct route_c*)arg)->con, buffer, sizeof(buffer), 0);
                    if (bytesrecv < 0) {
                        // if (bytesrecv == ECONNRESET...) {
                        
                        // } else {
                            fprintf(stderr, "recv(): %s.\n", strerror(errno));
                            goto FAILURE;
                        // }
                    } else if (bytesrecv == 0) {
                        printf("inbound disconnected.\n");

                        websocket_send(((struct route_c*)arg)->wscon, NULL, 0, CLOSE, true);

                        loop = !loop;
                        break;
                    }
                    
                    websocket_send(((struct route_c*)arg)->wscon, buffer, bytesrecv, BINARY, true);
                } else if (epe[i].data.u32 == ROUTE_WEBSOCKET) {
                    struct message msg = websocket_recv(((struct route_c*)arg)->wscon);

                    if (msg.opcode == CLOSE) {
                        printf("Websocket closed");
                        
                        if (msg.size > 0) {
                            uint16_t statuscode = 0;
                            memcpy(&statuscode, msg.buffer, sizeof(uint16_t));
                            statuscode = be16toh(statuscode);
                            printf(", status code: %i", statuscode);
    
                            char reason[msg.size - sizeof(statuscode) + 1];
                            memcpy(reason, msg.buffer + sizeof(statuscode), msg.size - sizeof(statuscode));
                            reason[sizeof(reason) - 1] = '\0';
                            printf(", reason: %s\n", reason);
                        } else if (msg.size > 123) {
                            printf(", CloseFrame size too big! Not reading...\n");
                        } else {
                            printf(", No close frame provided.\n");
                        }

                        websocket_send(((struct route_c*)arg)->wscon, NULL, 0, CLOSE, true);

                        loop = !loop;
                        free(msg.buffer);
                        break;
                    } else if (send(((struct route_c*)arg)->con, msg.buffer, msg.size, 0) < 0) {
                        fprintf(stderr, "send(): %s.\n", strerror(errno));
                        free(msg.buffer);
                        goto FAILURE;
                    }

                    free(msg.buffer);
                }
            }
        }
    }
    
    close(epollfd);
    return (void*)EXIT_SUCCESS;
FAILURE:
    close(epollfd);
    return (void*)EXIT_FAILURE;
}

int main() {
    printf("Starting local connection...\n");
    
    int socket = socket_bind(INADDR_ANY, 48375);
    int connection = socket_listen(socket);
    
    printf("Starting websocket connection...\n");
    struct connection ws_connection = websocket_connect(parse_url("ws://127.0.0.1:8000/connect"));
    
    struct route_c route_c;
    route_c.con = connection;
    route_c.wscon = ws_connection;

    pthread_t thread1;
    pthread_create(&thread1, NULL, &route, (void*)&route_c);

    void* return_val;
    pthread_join(thread1, &return_val);
    printf("Thread exited with exitcode %i\n", (int)return_val);
    
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
