#ifndef CLIENT_SOCKET_H
#define CLIENT_SOCKET_H

#include <stdint.h>
#if defined(_WIN32)
#include <windows.h>
#endif
//#include "common.h"

PLATFORM_REP_SOCKET socket_bind(uint32_t addr, uint16_t port);
#endif
