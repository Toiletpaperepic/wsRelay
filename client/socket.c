#if __WIN32__
#include <ws2tcpip.h>
#endif
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "socket.h"

int socket_bind(uint32_t addr, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "socket(): %s.\n", strerror(errno));
        return -1;
    }
    
#if !__WIN32__ 
    int option = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt(): %s.\n", strerror(errno));
        return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt(): %s.\n", strerror(errno));
        return -1;
    }
#endif

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = addr;

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        fprintf(stderr, "bind(): %s.\n", strerror(errno));
        return -1;
    }
    
    return fd;
}