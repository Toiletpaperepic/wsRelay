#if _WIN32
#include <winsock2.h>
#include <windows.h>
#include <wepoll.h>
#else
#include <sys/socket.h>
#include <sys/epoll.h>
#endif
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#define RESIZEBUFFER_CUSTOM_ERROR 1
#include "websocket.h"
#include "socket.h"
#include "common.h"
#include "http.h"
#include "args.h"

volatile sig_atomic_t status = 0;

static void catch_function(int signo) {
    printf("Recive interrupt. Now exiting, bye bye!\n");
    status = signo;
}

void version() {
    printf("wsRelay %s\n", __PROJECT_VERSION__);
    char* message = malloc(1);
    message[0] = '\0';
    make_user_agent(&message);
    printf("%s", message);
    free(message);
}

void help(struct Argument* firstarglist, struct Argument* secondarglist) {
    printf("Usage: wsrelay --out-url <url> [options]\n");
    print_description(firstarglist);
    print_description(secondarglist);
}

enum connection_type {
    ROUTE_CLIENT_CONNECTION, 
    ROUTE_SERVER_WEBSOCKET_CONNECTION
};

enum thread_error {
    CONTINUE,
    NORMAL_EXIT,
    EPOLL_ERROR,
    READ_ERROR,
    WRITE_ERROR,
    INBOUND_DISCONNECTED,
    OUTBOUND_DISCONNECTED,
    OUTBOUND_DISCONNECTED_SAFELY = NORMAL_EXIT,
};

struct routedata {
    struct parsed_url* out_url;
    int in_socket_fd;
    int out_websocket_fd;
    pthread_t thread;
};

void* route(void* ptrrd) {
    // immediately copy route data, otherwise you'll get data races between main and this thread.
    struct routedata rd;
    memcpy(&rd, ptrrd, sizeof(struct routedata));
    
    printf("Route thread started!\n");
    enum thread_error error = 0;

    // create a epoll file discriptor
#if _WIN32
    HANDLE epollfd = epoll_create1(0);
#else
    int epollfd = epoll_create1(0);
#endif
    if (epollfd < 0) {
        fprintf(stderr, "epoll_create1(): %s.\n", strerror(errno));
        error = EPOLL_ERROR;
    }

    // add file descriptors to the queue
    struct epoll_event epolleventlocalsocket;
    epolleventlocalsocket.data.u32 = ROUTE_CLIENT_CONNECTION;
    epolleventlocalsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rd.in_socket_fd, &epolleventlocalsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        error = EPOLL_ERROR;
    }

    struct epoll_event epolleventwebsocket;
    epolleventwebsocket.data.u32 = ROUTE_SERVER_WEBSOCKET_CONNECTION;
    epolleventwebsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rd.out_websocket_fd, &epolleventwebsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        error = EPOLL_ERROR;
    }

    while (status != SIGINT && error == 0) {
        struct epoll_event epe[2];
        
        printf("waiting for packets...\n");
        int fdevents = epoll_wait(epollfd, epe, sizeof(epe) / sizeof(struct epoll_event), 1000 * 5);

        if (fdevents < 0)  {
            fprintf(stderr, "epoll_wait(): %s.\n", strerror(errno));
            error = EPOLL_ERROR;
        } else if (fdevents == 0) {
            // not ready
        } else {
            for (int i = 0; i < fdevents; i++) {
                if (epe[i].data.u32 == ROUTE_CLIENT_CONNECTION) {
                    uint8_t buffer[1024] = {};
                    int bytesrecv = recv(rd.in_socket_fd, buffer, sizeof(buffer), 0);

                    if (bytesrecv < 0) {
                        fprintf(stderr, "recv(): %s.\n", strerror(errno));
                        error = EPOLL_ERROR; break;
                    } else if (bytesrecv == 0) {
                        printf("inbound disconnected.\n");

                        if (websocket_send(rd.out_websocket_fd, NULL, 0, CLOSE, true)) {
                            fprintf(stderr,"websocket_send() failed to send!");
                            error = WRITE_ERROR; break;
                        }

                        error = INBOUND_DISCONNECTED; break;
                    }
                    
                    if (websocket_send(rd.out_websocket_fd, buffer, bytesrecv, BINARY, true)) {
                        fprintf(stderr,"websocket_send() failed to send!");
                        error = WRITE_ERROR; break;
                    }
                } else if (epe[i].data.u32 == ROUTE_SERVER_WEBSOCKET_CONNECTION) {
                    struct message msg = websocket_recv(rd.out_websocket_fd);
                    if (!msg.error) {
                        assert(((struct message_data*)msg.msgdata)->opcode != CONTINUATION); // should've already been handled.
                        
                        if (((struct message_data*)msg.msgdata)->opcode == CLOSE) {
                            printf("Websocket closed");
                            
                            if (((struct message_data*)msg.msgdata)->size > 0) {
                                uint16_t statuscode = 0;
                                memcpy(&statuscode, ((struct message_data*)msg.msgdata)->buffer, sizeof(uint16_t));
#if _WIN32
                            statuscode = htons(statuscode);
#else
                            statuscode = be16toh(statuscode);
#endif
                                printf(", status code: %i", statuscode);
        
                                char reason[((struct message_data*)msg.msgdata)->size - sizeof(statuscode) + 1];
                                memcpy(reason, ((struct message_data*)msg.msgdata)->buffer + sizeof(statuscode), ((struct message_data*)msg.msgdata)->size - sizeof(statuscode));
                                reason[sizeof(reason) - 1] = '\0';
                                printf(", reason: %s\n", reason);
                            } else if (((struct message_data*)msg.msgdata)->size > 123) {
                                printf(", CloseFrame size too big! Not reading...\n");
                            } else {
                                printf(", No close frame provided.\n");
                            }

                            if (websocket_send(rd.out_websocket_fd, NULL, 0, CLOSE, true)) {
                                fprintf(stderr,"websocket_send() failed to send!");
                                free(((struct message_data*)msg.msgdata)->buffer);
                                error = WRITE_ERROR; break;
                            }

                            free(((struct message_data*)msg.msgdata)->buffer);
                            error = OUTBOUND_DISCONNECTED_SAFELY; break;
                        } else if (((struct message_data*)msg.msgdata)->opcode == TEXT) {
                            printf("Unsupported opcode.\n");
                        } else if (((struct message_data*)msg.msgdata)->opcode == PING) {
                            if (websocket_send(rd.out_websocket_fd, NULL, 0, PONG, true)) {
                                fprintf(stderr,"websocket_send() failed to send!");
                                free(((struct message_data*)msg.msgdata)->buffer);
                                error = WRITE_ERROR; break;
                            }
                        } else if (((struct message_data*)msg.msgdata)->opcode == PONG) {
                            // ...
                        } else if (((struct message_data*)msg.msgdata)->opcode == BINARY) {
                            if (send(rd.in_socket_fd, ((struct message_data*)msg.msgdata)->buffer, ((struct message_data*)msg.msgdata)->size, 0) < 0) {
                                fprintf(stderr, "send(): %s.\n", strerror(errno));
                                free(((struct message_data*)msg.msgdata)->buffer);
                                error = WRITE_ERROR; break;
                            }
                        }

                        free(((struct message_data*)msg.msgdata)->buffer);
                    } else {
                        fprintf(stderr, "websocket_recv() failed to receive!\n");
                        if (((struct message_data*)msg.msgdata)->buffer != NULL) // unknown state but lets make sure
                            free(((struct message_data*)msg.msgdata)->buffer);
                        error = READ_ERROR; break;
                    }
                }
            }
        }
    }

    printf("Route thread exiting...\n");
    // TODO: this would be more nicer if these were macros.
    if (close(rd.in_socket_fd) < 0 && close(rd.out_websocket_fd) < 0 && 
#if _WIN32
        epoll_close(epollfd)
#else
        close(epollfd)
#endif
        < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    enum thread_error* pointermemerror = malloc(sizeof(enum thread_error));
    memcpy(pointermemerror, &error, sizeof(enum thread_error));
    return pointermemerror;
}

int main(int argc, char *argv[]) {
    // make sure there is 0 required args here
    // first parse
    register_argument(argversion, NULL, "version", IS_BOOL, false, "Display wsRelay version information.")
    register_argument(arghelp, &argversion, "help", IS_BOOL, false, "Display this information.")
    
    // second parse
    register_argument(argouturl, NULL, "out-url", IS_STRING, true, "Specify where <port> will forward to.");
    register_argument(argport, &argouturl, "port", IS_UNSIGNED_INT, false, "Specify <port> will be set.");

    if (parse_args(argc, argv, &arghelp, true)) {
        return EXIT_FAILURE;
    }

    if ((bool)arghelp.value == true) {
        help(&arghelp, &argport);
        return EXIT_SUCCESS;
    }

    if ((bool)argversion.value == true) {
        version();
        return EXIT_SUCCESS;
    }
    
    if (parse_args(argc, argv, &argport, false)) {
        cleanup_args(&argport);
        return EXIT_FAILURE;
    }

    struct parsed_url purl = parse_url(argouturl.value);
    if (purl.protocol == unknown) {
        fprintf(stderr, "Unknown protocol.\n");
        cleanup_args(&argport);
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    }
    
    uint16_t port = 0; 
    if (argport.value == NULL) {
        port = 48375;
    } else if (*(int*)argport.value > UINT16_MAX) {
        printf("invalid port.\n");
        cleanup_args(&argport);
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    } else {
        port = *(int*)argport.value;
    }

    cleanup_args(&argport);
    
    printf("Starting local connection...\n");


#if !_WIN32
    struct sigaction a;
    a.sa_handler = catch_function;
    a.sa_flags = 0;
    sigemptyset( &a.sa_mask );
    if (sigaction( SIGINT, &a, NULL ) < 0) {
        fprintf(stderr, "An error occurred while setting up a signal handler! %s.\n", strerror(errno));
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    }
#endif

#if _WIN32
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
#endif

    int socket = socket_bind(INADDR_ANY, port);
    if (socket < 0) {
        fprintf(stderr, "socket failed to bind! exiting...\n");
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    }

    if (listen(socket, 0) < 0) {
        fprintf(stderr, "listen(): %s.\n", strerror(errno));
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    }
    
    unsigned int threadroutes_total = 1;
    struct routedata** threadroutes = malloc(threadroutes_total * sizeof(*threadroutes));
    int return_error = 0;
    
    // keep constantly looking for a new connection. when we do, pass it along to a another thread to handle it.
    while (status != SIGINT) {
        threadroutes[threadroutes_total - 1] = malloc(sizeof(struct routedata));
        threadroutes[threadroutes_total - 1]->out_url = &purl;
        threadroutes[threadroutes_total - 1]->in_socket_fd = accept(socket, NULL, NULL);
        if (threadroutes[threadroutes_total - 1]->in_socket_fd < 0) {
            fprintf(stderr, "failed to accept a new connection. %s.\n", strerror(errno));
            free(threadroutes[threadroutes_total - 1]);
            continue;
        }

        // start a new websocket connection
        printf("Starting websocket connection...\n");
        threadroutes[threadroutes_total - 1]->out_websocket_fd = websocket_connect(purl);
        if (threadroutes[threadroutes_total - 1]->out_websocket_fd < 0) {
            fprintf(stderr, "failed to accept a new websocket connection.\n");
            if (close(threadroutes[threadroutes_total - 1]->in_socket_fd)) {
                fprintf(stderr, "close(): %s.\n", strerror(errno));
                free(threadroutes[threadroutes_total - 1]); return_error = EXIT_FAILURE; break;
            }
            free(threadroutes[threadroutes_total - 1]);
            continue;
        }

        pthread_create(&threadroutes[threadroutes_total - 1]->thread, NULL, &route, (void*)threadroutes[threadroutes_total - 1]);
        
        resizebuffer(threadroutes, threadroutes_total * sizeof(*threadroutes), free(threadroutes[threadroutes_total - 1]); return_error = EXIT_FAILURE; break;);
        threadroutes_total++;
    }

    for (int i = 0; i < threadroutes_total - 1; i++) {
        enum thread_error* return_val;
        pthread_join(threadroutes[i]->thread, (void*)&return_val);
        free(threadroutes[i]);
        printf("Thread[%i] exited with exitcode %i\n", i, *return_val);
        free(return_val);
    }

    free(threadroutes);
    free((void*)purl.address);
    free((void*)purl.path);
    
    if (close(socket) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return_error = EXIT_FAILURE;
    }

#if _WIN32
    WSACleanup();
#endif

    return return_error != 0 ? EXIT_SUCCESS : return_error;
}