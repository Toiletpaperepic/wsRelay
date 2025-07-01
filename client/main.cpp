#include <stdio.h>
#include "websocket.h"

int main() {
    printf("Starting websocket connection...\n");

    connect("ws://localhost:8000/connect");
    run();

    return 0;
}