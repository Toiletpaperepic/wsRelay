#if __WIN32__
#include <winsock2.h>
#include <windows.h>
#include <wepoll.h>
#else
#include <sys/socket.h>
#include <sys/epoll.h>
#endif
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include "websocket.h"
#include "socket.h"
#include "args.h"