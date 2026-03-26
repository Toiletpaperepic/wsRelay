#include <stdint.h>
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#else
#include <netinet/in.h>
#endif

int socket_bind(uint32_t addr, uint16_t port);