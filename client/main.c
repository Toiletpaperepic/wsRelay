#if HAVE_WSAPOLL
// included later down
#elif HAVE_SYS_EPOLL_H
#include <sys/epoll.h>
#elif HAVE_POLL_H
#include <poll.h>
#else
#error No implementation found for poll()!
#endif
#if HAVE_CREATETHREAD
// already included in windows.h... somewhere?
#elif HAVE_PTHREAD_H
#include <pthread.h>
#else
#error No implementation found for multithreading!
#endif
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>
#include <stddef.h>
#include <errno.h>
#define RESIZEBUFFER_CUSTOM_ERROR 1
#include "wsrelay.h"
#include "socket.h"
#include "common.h"
#include "args.h"

volatile sig_atomic_t status = 0;

static void catch_function(int signo) {
    warn("Recive interrupt. Now exiting, bye bye!");
    status = signo;
}

void version() {
    print("wsRelay %s", __PROJECT_VERSION__);
    char* message = malloc(1);
    message[0] = '\0';
    make_user_agent(&message);
    printf("%s", message);
    free(message);
}

void help(const char* programname, struct Argument* firstarglist, struct Argument* secondarglist) {
    print("Usage: %s --out-url <url> [options]", programname);
    print_description(firstarglist);
    print_description(secondarglist);
}

enum connection_type {
    ROUTE_CLIENT_CONNECTION, 
    ROUTE_SERVER_WEBSOCKET_CONNECTION
};

enum thread_error {
    CONTINUE = -1,
    NORMAL_EXIT,
    POLL_ERROR,
    READ_ERROR,
    CLOSE_ERROR,
    WRITE_ERROR,
    INBOUND_DISCONNECTED,
    OUTBOUND_DISCONNECTED,
    OUTBOUND_DISCONNECTED_SAFELY = NORMAL_EXIT,
};

struct routedata {
    struct parsed_url* out_url;
    int in_socket_fd;
    int out_websocket_fd;
#if HAVE_CREATETHREAD
    HANDLE ThreadHandle;
    DWORD ThreadId;
#elif HAVE_PTHREAD_H
    pthread_t thread;
#endif
};

enum thread_error inbound(int in_socket_fd, int out_websocket_fd) {
    uint8_t buffer[1024] = {};
    int bytesrecv = recv(in_socket_fd, buffer, sizeof(buffer), 0);

    if (bytesrecv < 0) {
        error("recv(): %s.", strerror(errno));
        return READ_ERROR;
    } else if (bytesrecv == 0) {
        warn("inbound disconnected.");

        if (websocket_send(out_websocket_fd, NULL, 0, CLOSE, true)) {
            error("websocket_send() failed to send!");
            return WRITE_ERROR;
        }

        return INBOUND_DISCONNECTED;
    }
    
    if (websocket_send(out_websocket_fd, buffer, bytesrecv, BINARY, true)) {
        error("websocket_send() failed to send!");
        return WRITE_ERROR;
    }

    return CONTINUE;
}

enum thread_error outbound(int in_socket_fd, int out_websocket_fd) {
    struct message msg = websocket_recv(out_websocket_fd);
    if (!msg.error) {
        assert(((struct message_data*)msg.msgdata)->opcode != CONTINUATION); // should've already been handled.
        
        if (((struct message_data*)msg.msgdata)->opcode == CLOSE) {
            debug("Websocket closed");
            
            if (((struct message_data*)msg.msgdata)->size > 0) {
                uint16_t statuscode = 0;
                memcpy(&statuscode, ((struct message_data*)msg.msgdata)->buffer, sizeof(uint16_t));
#if defined(_WIN32)
                statuscode = htons(statuscode);
#else
                statuscode = be16toh(statuscode);
#endif
                debug("status code: %i", statuscode);
                
                size_t sizeofreason = ((struct message_data*)msg.msgdata)->size - sizeof(statuscode) + 1;
                char* reason = malloc(sizeofreason);
                memcpy(reason, (uint8_t*)(((struct message_data*)msg.msgdata)->buffer) + sizeof(statuscode), ((struct message_data*)msg.msgdata)->size - sizeof(statuscode));
                reason[sizeofreason - 1] = '\0';
                debug("reason: %s", reason);
                free(reason);
            } else if (((struct message_data*)msg.msgdata)->size > 123) {
                error("CloseFrame size too big! Not reading...");
            } else {
                warn("No close frame provided.");
            }

            if (websocket_send(out_websocket_fd, NULL, 0, CLOSE, true)) {
                error("websocket_send() failed to send!");
                free(((struct message_data*)msg.msgdata)->buffer);
                return WRITE_ERROR;
            }

            free(((struct message_data*)msg.msgdata)->buffer);
            return OUTBOUND_DISCONNECTED_SAFELY;
        } else if (((struct message_data*)msg.msgdata)->opcode == TEXT) {
            warn("Unsupported opcode.");
        } else if (((struct message_data*)msg.msgdata)->opcode == PING) {
            if (websocket_send(out_websocket_fd, NULL, 0, PONG, true)) {
                error("websocket_send() failed to send!");
                free(((struct message_data*)msg.msgdata)->buffer);
                return WRITE_ERROR;
            }
        } else if (((struct message_data*)msg.msgdata)->opcode == PONG) {
            // ...
        } else if (((struct message_data*)msg.msgdata)->opcode == BINARY) {
            if (send(in_socket_fd, ((struct message_data*)msg.msgdata)->buffer, ((struct message_data*)msg.msgdata)->size, 0) < 0) {
                error("send(): %s.", strerror(errno));
                free(((struct message_data*)msg.msgdata)->buffer);
                return WRITE_ERROR;
            }
        }

        free(((struct message_data*)msg.msgdata)->buffer);
    } else {
        error("websocket_recv() failed to receive!");
        if (((struct message_data*)msg.msgdata)->buffer != NULL) // unknown state but lets make sure
            free(((struct message_data*)msg.msgdata)->buffer);
        return READ_ERROR;
    }

    return CONTINUE;
}

#if HAVE_CREATETHREAD
DWORD route(void* ptrrd)
#elif HAVE_PTHREAD_H
void* route(void* ptrrd)
#endif
{
    // immediately copy route data, otherwise you'll get data races between main and this thread.
    struct routedata rd;
    memcpy(&rd, ptrrd, sizeof(struct routedata));
    
    debug("Route thread started!");
    enum thread_error error = 0;
    int timeout = 1000 * 5;

#if HAVE_SYS_EPOLL_H
    // create a epoll file discriptor
    int epollfd = epoll_create1(0);
    if (epollfd < 0) {
        error("epoll_create1(): %s.", strerror(errno));
        error = POLL_ERROR;
    }

    // add file descriptors to the queue
    struct epoll_event epolleventlocalsocket;
    epolleventlocalsocket.data.u32 = ROUTE_CLIENT_CONNECTION;
    epolleventlocalsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rd.in_socket_fd, &epolleventlocalsocket) < 0) {
        error("epoll_ctl(): %s.", strerror(errno));
        error = POLL_ERROR;
    }

    struct epoll_event epolleventwebsocket;
    epolleventwebsocket.data.u32 = ROUTE_SERVER_WEBSOCKET_CONNECTION;
    epolleventwebsocket.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rd.out_websocket_fd, &epolleventwebsocket) < 0) {
        error("epoll_ctl(): %s.", strerror(errno));
        error = POLL_ERROR;
    }

    while (status != SIGINT && error == 0) {
        struct epoll_event epe[2];
        
        // debug("waiting for packets...");
        int fdevents = epoll_wait(epollfd, epe, sizeof(epe) / sizeof(struct epoll_event), timeout);

        if (fdevents < 0)  {
            error("epoll_wait(): %s.", strerror(errno));
            error = POLL_ERROR;
        } else if (fdevents == 0) {
            // not ready
        } else {
            for (int i = 0; i < fdevents; i++) {
                if (epe[i].data.u32 == ROUTE_CLIENT_CONNECTION) {
                    enum thread_error result = inbound(rd.in_socket_fd, rd.out_websocket_fd);
                    if (result != CONTINUE) {
                        error = result; break;
                    }
                }
                else if (epe[i].data.u32 == ROUTE_SERVER_WEBSOCKET_CONNECTION) {
                    enum thread_error result = outbound(rd.in_socket_fd, rd.out_websocket_fd);
                    if (result != CONTINUE) {
                        error = result; break;
                    }
                }
            }
        }
    }
#elif HAVE_WSAPOLL || HAVE_POLL_H
    struct pollfd fds[2];

    fds[0].events = POLLIN;
    fds[1].events = POLLIN;
    fds[0].fd = rd.in_socket_fd;
    fds[1].fd = rd.out_websocket_fd;

    while (status != SIGINT && error == 0) {
        // debug("waiting for packets...");
        int pollret = 
#if HAVE_WSAPOLL
        WSAPoll
#elif HAVE_POLL_H
        poll
#endif
        (fds, sizeof(fds) / sizeof(struct pollfd), timeout);

        if (pollret < 0) {
            error( 
#if HAVE_WSAPOLL
                "WSAPoll(): %s."
#elif HAVE_POLL_H
                "poll(): %s."
#endif
                , strerror(errno));
            error = POLL_ERROR;
        } else if (pollret == 0) {
            // not ready
        } else  {
            for (int i = 0; i < sizeof(fds) / sizeof(struct pollfd); i++) {
                if (fds[i].revents & POLLIN) {
                    if (fds[i].fd == rd.in_socket_fd) {
                        enum thread_error result = inbound(rd.in_socket_fd, rd.out_websocket_fd);
                        if (result != CONTINUE) {
                            error = result; break;
                        }
                    } else if (fds[i].fd == rd.out_websocket_fd) {
                        enum thread_error result = outbound(rd.in_socket_fd, rd.out_websocket_fd);
                        if (result != CONTINUE) {
                            error = result; break;
                        }
                    }
                }
            }
        }
    }
#endif

    debug("Route thread exiting...");
    // TODO: this would be more nicer if these were macros.
    if (close(rd.in_socket_fd) < 0 && close(rd.out_websocket_fd) < 0  
#if HAVE_SYS_EPOLL_H
        && close(epollfd) < 0
#endif
    ) {
        error("close(): %s.", strerror(errno));
        error = CLOSE_ERROR;
    }
     
#if HAVE_CREATETHREAD
    return (DWORD)error;
#elif HAVE_PTHREAD_H
    enum thread_error* pointermemerror = malloc(sizeof(enum thread_error));
    memcpy(pointermemerror, &error, sizeof(enum thread_error));
    return (void*)pointermemerror;
#endif
}

int main(int argc, char *argv[]) {
#if LOGGER_COMPILE_OUT != 1 
    setuplogger();
#endif

    // make sure there is 0 required args here
    // first parse
    register_argument(argversion, NULL, "version", IS_BOOL, false, "Display program version information.")
    register_argument(arghelp, &argversion, "help", IS_BOOL, false, "Display this information.")
    
    // second parse
    register_argument(argaddress, NULL, "address", IS_STRING, true, "Specify where incoming trafic will relay to.");
    register_argument(argport, &argaddress, "port", IS_UNSIGNED_INT, false, "Bind to specify port (default: 48375).");

    if (parse_args(argc, argv, &arghelp, true)) {
        return EXIT_FAILURE;
    }

    if ((bool)arghelp.value == true) {
        help(argv[0], &arghelp, &argport);
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

    struct parsed_url purl = parse_url(argaddress.value);
    if (purl.protocol == unknown) {
        error("Unknown protocol.");
        cleanup_args(&argport);
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    }
    
    uint16_t port = 0; 
    if (argport.value == NULL) {
        port = 48375;
    } else if (*(int*)argport.value > UINT16_MAX) {
        error("invalid port.");
        cleanup_args(&argport);
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    } else {
        port = *(int*)argport.value;
    }

    cleanup_args(&argport);
    
    info("Starting local connection...");

#if defined(_WIN32)
    // todo: windows specific version

    WSADATA wsaData;
    int err;
    
    // begin loading Ws2_32.dll
    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        error("WSAStartup failed with error: %d", err);
        return EXIT_FAILURE;
    }
    
    //verify that we have the correct version
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        error("Could not find a usable version of Winsock.dll...");
        WSACleanup();
        return 1;
    } else {
        debug("Valid Winsock dll (v2.2) was found!");
    }
#else
    struct sigaction a;
    a.sa_handler = catch_function;
    a.sa_flags = 0;
    sigemptyset( &a.sa_mask );
    if (sigaction( SIGINT, &a, NULL ) < 0) {
        error("An error occurred while setting up a signal handler! %s.", strerror(errno));
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    }
#endif

    int socket = socket_bind(INADDR_ANY, port);
    if (socket < 0) {
        error("socket failed to bind! exiting...");
        free((void*)purl.address);
        free((void*)purl.path);
        return EXIT_FAILURE;
    }

    if (listen(socket, 0) < 0) {
        error("listen(): %s.", strerror(errno));
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
            error("failed to accept a new connection. %s.", strerror(errno));
            free(threadroutes[threadroutes_total - 1]);
            continue;
        }

        // start a new websocket connection
        info("Starting websocket connection...");
        threadroutes[threadroutes_total - 1]->out_websocket_fd = websocket_connect(purl);
        if (threadroutes[threadroutes_total - 1]->out_websocket_fd < 0) {
            error("failed to accept a new websocket connection.");
            close(threadroutes[threadroutes_total - 1]->in_socket_fd);
            free(threadroutes[threadroutes_total - 1]);
            continue;
        }

#if HAVE_CREATETHREAD
        threadroutes[threadroutes_total - 1]->ThreadHandle = CreateThread(
            NULL,
            0,
            route,
            (void*)threadroutes[threadroutes_total - 1],
            0,
            &threadroutes[threadroutes_total - 1]->ThreadId
        );
#elif HAVE_PTHREAD_H
        pthread_create(&threadroutes[threadroutes_total - 1]->thread, NULL, &route, (void*)threadroutes[threadroutes_total - 1]);
#endif

        threadroutes_total++;
        resizebuffer(threadroutes, threadroutes_total * sizeof(*threadroutes), free(threadroutes[threadroutes_total - 1]); return_error = EXIT_FAILURE; break;, false);
    }

    for (int i = 0; i < threadroutes_total - 1; i++) {
        enum thread_error* return_val;
#if HAVE_CREATETHREAD
        WaitForSingleObject(threadroutes[threadroutes_total - 1]->ThreadHandle, INFINITE);
        GetExitCodeThread(threadroutes[threadroutes_total - 1]->ThreadHandle, (void*)&return_val);
#elif HAVE_PTHREAD_H
        pthread_join(threadroutes[i]->thread, (void*)&return_val);
#endif
        free(threadroutes[i]);
        debug("Thread[%i] exited with exitcode %i", i, *return_val);
        free(return_val);
    }

    free(threadroutes);
    free((void*)purl.address);
    free((void*)purl.path);
    
    if (close(socket) < 0) {
        error("close(): %s.", strerror(errno));
        return_error = EXIT_FAILURE;
    }

#if defined(_WIN32)
    WSACleanup();
#endif

    return return_error != 0 ? EXIT_SUCCESS : return_error;
}