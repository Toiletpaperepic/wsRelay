// #include <cstdlib>
// #include <signal.h>
// #include <stdio.h>
#include "websocket.cpp"

// static bool interrupt = false;

// static void sigint_handler(int sig) {
// 	printf("exiting... %d", sig);
//     interrupt = true;
// }

int main() {
    // signal(SIGINT, sigint_handler);
    // printf("Starting connection...\n");
    
    websocket_handler handle;

    int error = handle.connect("ws://localhost:8000/connect");
    if (!error) {
        // printf("Created websocket endpoint connection Successfully...\n");
    } else {
        // printf("Websocket handler falled!\n");
        return 1;
    }

    handle.run();

    return 0;
}
