#include "main.h"

int main(int argc, char* argv[]) {
    // printf("Starting local connection...\n");

    // int socket = socket_bind(INADDR_ANY, 25565);
    // int connection = socket_listen(socket);

    // void *buffer[1024] = {};
    // while (strcmp((char *)buffer, "exit\n")) {
    //     memset(buffer, '\0', sizeof(buffer));
    //     check(recv(connection, buffer, sizeof(buffer), 0) < 0);
    //     printf("%s: Message from client: %s\n", argv[0], (char *)buffer);
    // }
    
    // check(close(connection) < 0);
    // check(close(socket) < 0);

    printf("Starting websocket connection...\n");

    struct connection connection = websocket_connect(parse_url("ws://127.0.0.1:8000/connect"));

    {
        char buffer[1024] = {};
        websocket_recv(connection, buffer, sizeof(buffer));
        printf("%s: message from server %s\n", argv[0], buffer);
        memset(buffer, '\0', sizeof(buffer));

    }

    for (int i = 0; i < 1; i++) {
        char header[2] = {};
        websocket_recv(connection, header, sizeof(header));

        int FIN = (header[0] & 0b10000000) != 0;
        printf("FIN: %i\n", FIN);
        assert(FIN == 1);

        int opcode = header[0] & 0b00001111;
        printf("opcode: %i\n", opcode);
        assert(opcode == 1 || opcode == 2);

        int masked = (header[1] & 0b10000000) != 0;
        printf("masked: %i\n", masked);
        assert(masked == 0);

        unsigned int payload_size = header[1] & 0b01111111;
        printf("payload size: %i\n", payload_size);

        memset(header, '\0', sizeof(header));

        char buffer[payload_size] = {};

        websocket_recv(connection, buffer, sizeof(buffer));
        printf("%s: message from server %s\n", argv[0], buffer);
        memset(buffer, '\0', sizeof(buffer));
    }

    close(connection.fd);

    free_purl(connection.url);

    return 0;
}