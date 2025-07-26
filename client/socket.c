#include <unistd.h>
#include "socket.h"
#include "check.h"

int socket_bind(in_addr_t s_addr, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    check2(fd < 0);

    // allow reuse
    int option = 1;
    check(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0);
    check(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(int)) < 0);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = s_addr;

    check2(bind(fd, (struct sockaddr*)&address, sizeof(address)) != 0);
    
    return fd;
}

int socket_listen(int fd) {
    check(listen(fd, 1) < 0);
    int connection = accept(fd, NULL, NULL);
    check(connection < 0);
    return connection;
}
