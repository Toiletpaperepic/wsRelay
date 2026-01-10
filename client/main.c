#include "main.h"
#include <stdio.h>

volatile sig_atomic_t status = 0;

static void catch_function(int signo) {
    printf("Recive interrupt. Now exiting, bye bye!\n");
    status = signo;
}

#define ROUTE_CLIENT_CONNECTION 0
#define ROUTE_WEBSOCKET 1

struct routedata {
    const char* out_url;
    int in_socket;
};

void* route(void* ptrrd) {
    printf("Route thread started!\n");
    
    // immediately copy route data, otherwise you'll get data races between main and this thread.
    struct routedata rd;
    memcpy(&rd, ptrrd, sizeof(struct routedata));

    // start a new websocket connection
    printf("Starting websocket connection...\n");
    struct connection out_websocket = websocket_connect(parse_url(rd.out_url));
    // printf("started a new connection route to %s:%i/%s\n", out_websocket.url.address, out_websocket.url.port, out_websocket.url.path);
    free((void*)out_websocket.url.address);
    free((void*)out_websocket.url.path);

    // create a epoll file discriptor
#if __WIN32__
    HANDLE epollfd = epoll_create1(0);
#else
    int epollfd = epoll_create1(0);
#endif
    if (epollfd < 0) {
        fprintf(stderr, "epoll_create1(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    // add file descriptors to the queue
    struct epoll_event epolleventlocalsocket;
    epolleventlocalsocket.data.u32 = ROUTE_CLIENT_CONNECTION;
    epolleventlocalsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rd.in_socket, &epolleventlocalsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    struct epoll_event epolleventwebsocket;
    epolleventwebsocket.data.u32 = ROUTE_WEBSOCKET;
    epolleventwebsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, out_websocket.fd, &epolleventwebsocket) < 0) {
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
                    int bytesrecv = recv(rd.in_socket, buffer, sizeof(buffer), 0);

                    if (bytesrecv < 0) {
                        // if (bytesrecv == ECONNRESET...) {
                        
                        // } else {
                            fprintf(stderr, "recv(): %s.\n", strerror(errno));
                            goto FAILURE;
                        // }
                    } else if (bytesrecv == 0) {
                        printf("inbound disconnected.\n");

                        websocket_send(out_websocket, NULL, 0, CLOSE, true);

                        loop = !loop;
                        break;
                    }
                    
                    websocket_send(out_websocket, buffer, bytesrecv, BINARY, true);
                } else if (epe[i].data.u32 == ROUTE_WEBSOCKET) {
                    struct message msg = websocket_recv(out_websocket);
                    assert(msg.opcode != CONTINUATION); // should've already been handled.
                    
                    if (msg.opcode == CLOSE) {
                        printf("Websocket closed");
                        
                        if (msg.size > 0) {
                            uint16_t statuscode = 0;
                            memcpy(&statuscode, msg.buffer, sizeof(uint16_t));
#if __WIN32__
                            statuscode = htons(statuscode);
#else
                            statuscode = be16toh(statuscode);
#endif
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

                        websocket_send(out_websocket, NULL, 0, CLOSE, true);

                        loop = !loop;
                        free(msg.buffer);
                        break;
                    } else if (msg.opcode == TEXT) {
                        printf("Unsupported opcode.\n");
                    } else if (msg.opcode == PING) {
                        websocket_send(out_websocket, NULL, 0, PONG, true);
                    } else if (msg.opcode == PONG) {
                        // ...
                    } else if (msg.opcode == BINARY) {
                        if (send(rd.in_socket, msg.buffer, msg.size, 0) < 0) {
                            fprintf(stderr, "send(): %s.\n", strerror(errno));
                            free(msg.buffer);
                            goto FAILURE;
                        }
                    }

                    free(msg.buffer);
                }
            }ws://127.0.0.1:8000/connect
        }
    }

    printf("Route thread exiting...\n");
#if __WIN32__
    if (close(rd.in_socket) < 0 && close(out_websocket.fd) < 0 && epoll_close(epollfd) < 0) {
#else
    if (close(rd.in_socket) < 0 && close(out_websocket.fd) < 0 && close(epollfd) < 0) {
#endif
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    return (void*)EXIT_SUCCESS;
FAILURE:
#if __WIN32__
    if (close(rd.in_socket) < 0 && close(out_websocket.fd) < 0 && epoll_close(epollfd) < 0) {
#else
    if (close(rd.in_socket) < 0 && close(out_websocket.fd) < 0 && close(epollfd) < 0) {
#endif
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    return (void*)EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    register_argument(arg0, NULL, "out-url", IS_STRING, true);
    register_argument(arg1, &arg0, "port", IS_UNSIGNED_INT, false);

    if (parse_args(argc, argv, &arg1)) {
        return EXIT_FAILURE;
    }

    printf("Starting local connection...\n");

#if __WIN32__
    WSADATA wsaData;
    int err;

    // begin loading Ws2_32.dll
    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return EXIT_FAILURE;
    }

    //verify that we have the correct version
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        fprintf(stderr, "Could not find a usable version of Winsock.dll...\n");
        WSACleanup();
        return 1;
    } else {
        printf("Valid Winsock dll (v2.2) was found!\n");
    }
#else
    struct sigaction a;
    a.sa_handler = catch_function;
    a.sa_flags = 0;
    sigemptyset( &a.sa_mask );
    
    if (sigaction(SIGINT, &a, NULL) < 0) { // https://stackoverflow.com/questions/15617562/sigintctrlc-doesnt-interrupt-accept-call
        fprintf(stderr, "An error occurred while setting up a signal handler.\n");
        return EXIT_FAILURE;
    }
#endif
    
    unsigned int threads_total = 1;
    pthread_t** threads = malloc(threads_total * sizeof(*threads));

    int socket = socket_bind(INADDR_ANY, arg1.value == NULL ? 48375 : *(int*)arg1.value);
    if (socket < 0) {
        fprintf(stderr, "socket failed to bind.\n");
        return EXIT_FAILURE;
    }

    // keep constantly looking for a new connection. when we do, pass it along to a another thread to handle it.
    while (status != SIGINT) {
        if (socket_listen(socket) == EXIT_FAILURE) {
            fprintf(stderr, "failed to listen to socket.\n");
            break;
        }
        
        struct routedata rd;
        rd.in_socket = socket_accept(socket);
        if (rd.in_socket < 0) {
            if (status != SIGINT) {
                fprintf(stderr, "failed to accept a new connection.\n");
            }
            break;
        }
        rd.out_url = (char*)arg0.value;

        threads[threads_total - 1] = malloc(sizeof(pthread_t*));
        pthread_create(threads[threads_total - 1], NULL, &route, (void*)&rd);
        
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

#if __WIN32__
    WSACleanup();
#endif

    struct Argument* nextarg = &arg0;
    while (true) {
        if (nextarg->value != NULL && nextarg->type != IS_BOOL) {
            // printf("freed %s\n", nextarg->name);
            free(nextarg->value);
        } else {
            // printf("not freed %s\n", nextarg->name);
        }
    
        if (nextarg->next == NULL) {
            break;
        } else {
            nextarg = nextarg->next;
        }
    }

    return EXIT_SUCCESS;
}