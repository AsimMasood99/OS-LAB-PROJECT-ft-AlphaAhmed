#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "parser.h"

int main()
{
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Replace with server IP
    server_addr.sin_port = htons(8080);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        
        // Send data to the server
        char command[256]; 
        printf("Enter Command: "); 
        fgets(command, sizeof(command), stdin);
        printf("\nThe command is: %s\n", command);
        char* parsed;
        PARSER(command,parsed);
        printf("responce from paser: %s",parsed);
        
        const char *message = "Hello from client!";
        
        send(client_socket, message, strlen(message), 0);

        // Receive data from the server
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received < 0)
        {
            perror("Error receiving data");
            exit(EXIT_FAILURE);
        }

        printf("Received from server: %s\n", buffer);
    }
    close(client_socket);
    return 0;
}