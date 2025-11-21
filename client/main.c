#include "main.h"

#define ROUTE_LOCAL_SOCKET 0
#define ROUTE_WEBSOCKET 1

void* route(void* local_socket) {
    printf("Route thread started!\n");

    // start a new websocket connection
    printf("Starting websocket connection...\n");
    struct connection ws_connection = websocket_connect(parse_url("ws://127.0.0.1:8000/connect"));
    // printf("started a new connection route to %s:%i/%s\n", ws_connection.url.address, ws_connection.url.port, ws_connection.url.path);
    free((void*)ws_connection.url.address);
    free((void*)ws_connection.url.path);

    // create a epoll file discriptor
    int epollfd = epoll_create1(0);
    if (epollfd < 0) {
        fprintf(stderr, "epoll_create1(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    // add file descriptors to the queue
    struct epoll_event epolleventlocalsocket;
    epolleventlocalsocket.data.u32 = ROUTE_LOCAL_SOCKET;
    epolleventlocalsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, *(int*)local_socket, &epolleventlocalsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    struct epoll_event epolleventwebsocket;
    epolleventwebsocket.data.u32 = ROUTE_WEBSOCKET;
    epolleventwebsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ws_connection.fd, &epolleventwebsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    bool loop = true;
    while (loop) {
        struct epoll_event epe[2];
        
        printf("waiting for packets...\n");
        int fdevents = epoll_wait(epollfd, epe, sizeof(epe) / sizeof(struct epoll_event), -1);

        if (fdevents < 0)  {
            fprintf(stderr, "epoll_wait(): %s.\n", strerror(errno));
            goto FAILURE;
        } else if (fdevents == 0) {
            // not ready
        } else {
            for (int i = 0; i < fdevents; i++) {
                if (epe[i].data.u32 == ROUTE_LOCAL_SOCKET) {
                    uint8_t buffer[1024] = {};
                    int bytesrecv = recv(*(int*)local_socket, buffer, sizeof(buffer), 0);

                    if (bytesrecv < 0) {
                        // if (bytesrecv == ECONNRESET...) {
                        
                        // } else {
                            fprintf(stderr, "recv(): %s.\n", strerror(errno));
                            goto FAILURE;
                        // }
                    } else if (bytesrecv == 0) {
                        printf("inbound disconnected.\n");

                        websocket_send(ws_connection, NULL, 0, CLOSE, true);

                        loop = !loop;
                        break;
                    }
                    
                    websocket_send(ws_connection, buffer, bytesrecv, BINARY, true);
                } else if (epe[i].data.u32 == ROUTE_WEBSOCKET) {
                    struct message msg = websocket_recv(ws_connection);
                    assert(msg.opcode != CONTINUATION); // should've already been handled.
                    
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

                        websocket_send(ws_connection, NULL, 0, CLOSE, true);

                        loop = !loop;
                        free(msg.buffer);
                        break;
                    } else if (msg.opcode == TEXT) {
                        printf("Unsupported opcode.\n");
                    } else if (msg.opcode == PING) {
                        websocket_send(ws_connection, NULL, 0, PONG, true);
                    } else if (msg.opcode == PONG) {
                        // ...
                    } else if (msg.opcode == BINARY) {
                        if (send(*(int*)local_socket, msg.buffer, msg.size, 0) < 0) {
                            fprintf(stderr, "send(): %s.\n", strerror(errno));
                            free(msg.buffer);
                            goto FAILURE;
                        }
                    }

                    free(msg.buffer);
                }
            }
        }
    }

    if (close(*(int*)local_socket) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    if (close(ws_connection.fd) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    if (close(epollfd) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    return (void*)EXIT_SUCCESS;
FAILURE:
    if (close(*(int*)local_socket) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    if (close(ws_connection.fd) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    if (close(epollfd) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    return (void*)EXIT_FAILURE;
}

int main() {
    printf("Starting local connection...\n");
    
    int socket = socket_bind(INADDR_ANY, 48375);
    int new_connection = socket_listen(socket);

    pthread_t thread1;
    pthread_create(&thread1, NULL, &route, (void*)&new_connection);

    int return_val;
    pthread_join(thread1, (void*)&return_val);
    printf("Thread exited with exitcode %i\n", return_val);
    
    if (close(socket) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}