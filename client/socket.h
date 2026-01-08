#if __WIN32__
#include <winsock2.h>
#include <windows.h>
#else
#include <netinet/in.h>
#endif

int socket_bind(uint32_t addr, uint16_t port);
bool socket_listen(int fd);
int socket_accept(int fd);