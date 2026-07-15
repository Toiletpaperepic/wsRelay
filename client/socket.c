#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif
#include <string.h>
#include <errno.h>
#include "other.h"
#include "socket.h"

PLATFORM_REP_SOCKET socket_bind(uint32_t addr, uint16_t port) {
    PLATFORM_REP_SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        error("socket(): %s.", strerror(errno));
        return -1;
    }
    
#if !defined(_WIN32) 
    int option = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0) {
        error("setsockopt(): %s.", strerror(errno));
        return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(int)) < 0) {
        error("setsockopt(): %s.", strerror(errno));
        return -1;
    }
#endif

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = addr;

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        error("bind(): %s.", strerror(errno));
        return -1;
    }
    
    return fd;
}
