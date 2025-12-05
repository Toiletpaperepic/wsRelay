#include <netinet/in.h>

int socket_bind(in_addr_t s_addr, uint16_t port);
bool socket_listen(int fd);
int socket_accept(int fd);