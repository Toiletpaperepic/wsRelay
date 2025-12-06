#include "main.h"
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t status = 0;

static void catch_function(int signo) {
    printf("Recive interrupt. Now exiting, bye bye!\n");
    status = signo;
}

#define ROUTE_CLIENT_CONNECTION 0
#define ROUTE_WEBSOCKET 1

void* route(void* client_connection_pointer) {
    printf("Route thread started!\n");
    
    // immediately copy the client pointer, otherwise you'll get data races between main and this thread.
    int cilent_connection = 0;
    memcpy(&cilent_connection, client_connection_pointer, sizeof(int));

    // start a new websocket connection
    printf("Starting websocket connection...\n");
    struct connection ws_connection = websocket_connect(parse_url("ws://192.168.68.72:8000/connect"));
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
    epolleventlocalsocket.data.u32 = ROUTE_CLIENT_CONNECTION;
    epolleventlocalsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, cilent_connection, &epolleventlocalsocket) < 0) {
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
                if (epe[i].data.u32 == ROUTE_CLIENT_CONNECTION) {
                    uint8_t buffer[1024] = {};
                    int bytesrecv = recv(cilent_connection, buffer, sizeof(buffer), 0);

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
                        if (send(cilent_connection, msg.buffer, msg.size, 0) < 0) {
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

    printf("Route thread exiting...\n");

    if (close(cilent_connection) < 0) {
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
    if (close(cilent_connection) < 0) {
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

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    printf("Starting local connection...\n");

    struct sigaction a;
    a.sa_handler = catch_function;
    a.sa_flags = 0;
    sigemptyset( &a.sa_mask );
     
    if (sigaction(SIGINT, &a, NULL) < 0) { // https://stackoverflow.com/questions/15617562/sigintctrlc-doesnt-interrupt-accept-call
        fprintf(stderr, "An error occurred while setting up a signal handler.\n");
        return EXIT_FAILURE;
    }
    
    unsigned int threads_total = 1;
    pthread_t** threads = malloc(threads_total * sizeof(*threads));

    int socket = socket_bind(INADDR_ANY, 48375);
    if (socket < -1) {
        fprintf(stderr, "socket failed to bind.\n");
        return EXIT_FAILURE;
    }

    // keep constantly looking for a new connection. when we do, pass it along to a another thread to handle it.
    while (status != SIGINT) {
        if (socket_listen(socket) == EXIT_FAILURE) {
            fprintf(stderr, "failed to listen to socket.\n");
            break;
        }
        
        int new_connection = socket_accept(socket);

        if (new_connection < 0) {
            if (status != SIGINT) {
                fprintf(stderr, "failed to accept a new connection.\n");
            }
            break;
        }

        threads[threads_total - 1] = malloc(sizeof(pthread_t*));
        pthread_create(threads[threads_total - 1], NULL, &route, (void*)&new_connection);
        
        threads_total++;
        pthread_t** tmp = realloc(threads, threads_total * sizeof(*threads));
        if (tmp == NULL) {
            fprintf(stderr, "realloc(): Unknown reason.\n");
            free(threads);
            return EXIT_FAILURE;
        }
        threads = tmp;
    }

    for (int i = 0; i < threads_total - 1; i++) {
        int return_val;
        pthread_join(*threads[i], (void*)&return_val);
        free(threads[i]);
        printf("Thread[%i] exited with exitcode %i\n", i, return_val);
    }

    free(threads);
    
    if (close(socket) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}