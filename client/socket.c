#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "socket.h"

int socket_bind(in_addr_t s_addr, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "socket(): %s.\n", strerror(errno));
        exit(errno);
    }

    int option = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt(): %s.\n", strerror(errno));
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt(): %s.\n", strerror(errno));
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = s_addr;

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        fprintf(stderr, "bind(): %s.\n", strerror(errno));
        exit(errno);
    }
    
    return fd;
}

int socket_listen(int fd) {
    if (listen(fd, 1) < 0) {
        fprintf(stderr, "listen(): %s.\n", strerror(errno));
        exit(errno);
    }

    int connection = accept(fd, NULL, NULL);
    if (connection < 0) {
        fprintf(stderr, "accept(): %s.\n", strerror(errno));
        exit(errno);
    }

    return connection;
}
