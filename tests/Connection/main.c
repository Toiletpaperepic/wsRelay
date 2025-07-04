#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define check(expression, reason)   \
    {   \
        int result = expression;    \
        if (result) {    \
            if (strcmp(reason, "") == 0) {   \
                printf("check failed on %s:%i, got %i reason: unknown\n", __FILE__, __LINE__, result); \
            } else {    \
                printf("check failed on %s:%i, got %i reason: %s\n", __FILE__, __LINE__, result, reason); \
            }   \
            return 1;   \
        }   \
        /* TODO: write kill() calls to pid(1,2,3) */    \
    }

int main() {    
    // split apart server, client, and the actual test
    int pid1 = fork();
    check(pid1 < 0, "pid1 = fork() failed.")
    
    if (pid1 == 0) {
        // run the server
        check(execlp(SERVER_FILE, SERVER_FILE, NULL), "");
    } else {
        // fork again!
        int pid2 = fork();
        check(pid2 < 0, "pid2 = fork() failed.");
        
        if(pid2 == 0) {
            sleep(5);
            // run the client
            check(execlp(CLIENT_FILE, CLIENT_FILE, NULL), "");
        } else {
            const char* testmesssage = "Hello, World!";

            // another fork?
            int pid3 = fork();
            check(pid3 < 0, "pid3 = fork() failed.")
            
            if (pid3 == 0) {
                // now listen for wsp-server.
                // we will act like this is a normal dedicated game server
                // taken from client/socket.cpp
                printf("now listening for wsp-server\n");
                
                int startpointSocket = socket(AF_INET, SOCK_STREAM, 0);
            
                struct sockaddr_in serverAddress;
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(25565);
                serverAddress.sin_addr.s_addr = INADDR_ANY;
            
                check(bind(startpointSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)), ""); 
                
                // now we wait
                check(listen(startpointSocket, 1), "");
                int clientSocket = accept(startpointSocket, NULL, NULL);
                
                char buffer[1024] = {0};
                recv(clientSocket, buffer, sizeof(buffer), 0);
                printf("wsp-connection-test: Message from client: %s\n", buffer);
                
                if (testmesssage == buffer) {
                    printf("Passed!\n");
                }
                
                close(startpointSocket);
                return 0;
            } else {
                sleep(10);
                // now add a endpoint socket
                // acts like a game client connecting to wsp-client
                printf("now listening for wsp-client\n");
                
                struct in_addr addr;
                
                inet_aton("127.0.0.1", &addr);
                
                struct sockaddr_in serverAddress;
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(54332);
                serverAddress.sin_addr.s_addr = addr.s_addr;
                
                int endpointSocket = socket(AF_INET, SOCK_STREAM, 0);

                check(connect(endpointSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)), "");
                
                send(endpointSocket, testmesssage, strlen(testmesssage), 0);
                printf("Sent!\n");

                close(endpointSocket);
            }
            
            check(wait(&pid3) < 0, "");
            check(wait(&pid2) < 0, "");
            check(wait(&pid1) < 0, "");
            printf("pid1 == %i\n", pid1);
            printf("pid2 == %i\n", pid2);
            printf("pid3 == %i\n", pid3);

            printf("done!\n");
        }
    }

    return 0;
}