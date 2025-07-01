#include <netinet/in.h>

int socket_bind(in_addr_t address, uint16_t port);
int socket_listen(int startPointSocket);