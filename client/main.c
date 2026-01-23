#define RESIZEBUFFER_CUSTOM_ERROR 1
#include "main.h"

volatile sig_atomic_t status = 0;

static void catch_function(int signo) {
    printf("Recive interrupt. Now exiting, bye bye!\n");
    status = signo;
}

#define ROUTE_CLIENT_CONNECTION 0
#define ROUTE_WEBSOCKET 1

struct routedata {
    struct parsed_url* out_url;
    int in_socket_fd;
    int out_websocket_fd;
    pthread_t thread;
    bool done;
};

void* route(void* ptrrd) {
    printf("Route thread started!\n");
    
    // immediately copy route data, otherwise you'll get data races between main and this thread.
    struct routedata rd;
    memcpy(&rd, ptrrd, sizeof(struct routedata));

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
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rd.in_socket_fd, &epolleventlocalsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    struct epoll_event epolleventwebsocket;
    epolleventwebsocket.data.u32 = ROUTE_WEBSOCKET;
    epolleventwebsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rd.out_websocket_fd, &epolleventwebsocket) < 0) {
        fprintf(stderr, "epoll_ctl(): %s.\n", strerror(errno));
        goto FAILURE;
    }

    bool loop = true;
    while (status != SIGINT && loop) {
        struct epoll_event epe[10];
        
        printf("waiting for packets...\n");
        int fdevents = epoll_wait(epollfd, epe, sizeof(epe) / sizeof(struct epoll_event), 1000 * 5);

        if (fdevents < 0)  {
            fprintf(stderr, "epoll_wait(): %s.\n", strerror(errno));
            goto FAILURE;
        } else if (fdevents == 0) {
            // not ready
        } else {
            for (int i = 0; i < fdevents; i++) {
                if (epe[i].data.u32 == ROUTE_CLIENT_CONNECTION) {
                    uint8_t buffer[1024] = {};
                    int bytesrecv = recv(rd.in_socket_fd, buffer, sizeof(buffer), 0);

                    if (bytesrecv < 0) {
                        // if (bytesrecv == ECONNRESET...) {
                        
                        // } else {
                            fprintf(stderr, "recv(): %s.\n", strerror(errno));
                            goto FAILURE;
                        // }
                    } else if (bytesrecv == 0) {
                        printf("inbound disconnected.\n");

                        websocket_send(rd.out_websocket_fd, NULL, 0, CLOSE, true);

                        loop = !loop;
                        break;
                    }
                    
                    websocket_send(rd.out_websocket_fd, buffer, bytesrecv, BINARY, true);
                } else if (epe[i].data.u32 == ROUTE_WEBSOCKET) {
                    struct message msg = websocket_recv(rd.out_websocket_fd);
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

                        websocket_send(rd.out_websocket_fd, NULL, 0, CLOSE, true);

                        loop = !loop;
                        free(msg.buffer);
                        break;
                    } else if (msg.opcode == TEXT) {
                        printf("Unsupported opcode.\n");
                    } else if (msg.opcode == PING) {
                        websocket_send(rd.out_websocket_fd, NULL, 0, PONG, true);
                    } else if (msg.opcode == PONG) {
                        // ...
                    } else if (msg.opcode == BINARY) {
                        if (send(rd.in_socket_fd, msg.buffer, msg.size, 0) < 0) {
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
    ((struct routedata*)ptrrd)->done = true;

    if (close(rd.in_socket_fd) < 0 && close(rd.out_websocket_fd) < 0 && close(epollfd) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    return (void*)EXIT_SUCCESS;
FAILURE:
    if (close(rd.in_socket_fd) < 0 && close(rd.out_websocket_fd) < 0 && close(epollfd) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    return (void*)EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    register_argument(arg0, NULL, "out-url", IS_STRING, true);
    register_argument(arg1, &arg0, "port", IS_UNSIGNED_INT, false);
    
    if (parse_args(argc, argv, &arg1)) {
        cleanup_args(&arg1);
        return EXIT_FAILURE;
    }

    struct parsed_url purl = parse_url(arg0.value);
    if (purl.protocol == unknown) {
        fprintf(stderr, "Unknown protocol.\n");
        cleanup_args(&arg1);
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    }
    
    uint16_t port = 0; 
    if (arg1.value == NULL) {
        port = 48375;
    } else if (*(int*)arg1.value > UINT16_MAX) {
        printf("invalid port.\n");
        cleanup_args(&arg1);
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    } else {
        port = *(int*)arg1.value;
    }

    cleanup_args(&arg1);
    
    unsigned int threadroutes_total = 1;
    struct routedata** threadroutes = malloc(threadroutes_total * sizeof(*threadroutes));
    
    printf("Starting local connection...\n");

    int socket = socket_bind(INADDR_ANY, port);
    if (socket < 0) {
        fprintf(stderr, "socket failed to bind! exiting...\n");
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, catch_function) < 0) {
        fprintf(stderr, "An error occurred while setting up a signal handler! %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
    
    if (listen(socket, 0) < 0) {
        fprintf(stderr, "listen(): %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int return_error = 0;

    // keep constantly looking for a new connection. when we do, pass it along to a another thread to handle it.
    while (status != SIGINT) {
        struct pollfd fd;
        
        fd.fd = socket;
        fd.events = POLLIN;

        int ret = poll(&fd, 1, 1000 * 5);
        if (ret < 0) {
            fprintf(stderr, "poll(): %s.\n", strerror(errno));
            return_error = EXIT_FAILURE;
            break;
	    }

        if (fd.revents & POLLIN) {
            threadroutes[threadroutes_total - 1] = malloc(sizeof(struct routedata));
            threadroutes[threadroutes_total - 1]->done = false;
            threadroutes[threadroutes_total - 1]->out_url = &purl;
            threadroutes[threadroutes_total - 1]->in_socket_fd = accept(socket, NULL, NULL);

            // start a new websocket connection
            printf("Starting websocket connection...\n");
            threadroutes[threadroutes_total - 1]->out_websocket_fd = websocket_connect(purl);

            if (threadroutes[threadroutes_total - 1]->in_socket_fd < 0) {
                fprintf(stderr, "failed to accept a new connection. %s.\n", strerror(errno));
                return_error = EXIT_FAILURE;
                break;
            }
    
            pthread_create(&threadroutes[threadroutes_total - 1]->thread, NULL, &route, (void*)threadroutes[threadroutes_total - 1]);
            
            threadroutes_total++;
            resizebuffer(threadroutes, threadroutes_total * sizeof(*threadroutes), return_error = EXIT_FAILURE; break;);
        } else {
            // not ready
        }
    }

    for (int i = 0; i < threadroutes_total - 1; i++) {
        int return_val;
        pthread_join(threadroutes[i]->thread, (void*)&return_val);
        free(threadroutes[i]);
        printf("Thread[%i] exited with exitcode %i\n", i, return_val);
    }
    
    free(threadroutes);
    free((void*)purl.address);
    free((void*)purl.path);
    
    if (close(socket) < 0) {
        fprintf(stderr, "close(): %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return return_error != 0 ? EXIT_SUCCESS : return_error;
}