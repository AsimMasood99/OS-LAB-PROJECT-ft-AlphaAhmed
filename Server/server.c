#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <sys/stat.h>

int isSpaceAvailable(const char *path, int maxSpace)
{
    return 1;
}

int main()
{
    int server_socket, new_socket, client_addr_len;
    int recieving = 0;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(3001); // Replace with desired port

    // Bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0)
    {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 3001...\n");

    // Accept connections
    while (1)
    {
        client_addr_len = sizeof(client_addr);
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_socket < 0)
        {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        // Receive data from the client
        while (1)
        {
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = recv(new_socket, buffer, sizeof(buffer), 0);
            if (bytes_received < 0)
            {
                perror("Error receiving data");
                break;
            }
            if (bytes_received == 0)
            {
                printf("Client disconnected\n");
                break;
            }
            cJSON* jsonCommad = cJSON_Parse(buffer);
            cJSON* commandType = cJSON_GetObjectItem(jsonCommad,"command");
            printf("command: %s\n", commandType->valuestring);

            char folder_name[256] = "./Server/Storage/";

            strcat(folder_name, inet_ntoa(client_addr.sin_addr));
            if (strcmp(commandType->valuestring, "upload") == 0)
            {   
                
                struct stat folder = {0};

                if (stat(folder_name, &folder) == -1)
                {
                    mkdir(folder_name, 0700);
                    char *response = "{\"status\":\"ready\", \"command\":\"upload}";
                    send(new_socket, response, strlen(response), 0);
                }
                else
                {
                    if (isSpaceAvailable(folder_name, 10000))
                    {
                        char *response = "{\"status\":\"ready\", \"command\":\"upload}";
                        send(new_socket, response, strlen(response), 0);
                    }
                    else
                    {
                        char *response = "{\"status\":\"failed\", \"error\":\"stoage full\"}";
                        send(new_socket, response, strlen(response), 0);
                    }
                }
            }

            else if (strcmp(commandType->valuestring, "close") == 0)
            {
                close(new_socket);
                close(server_socket);
                return 0;
            }

            // Send a response to the client
            // const char *response = "Hello from server!";
            // send(new_socket, response, strlen(response), 0);
        }
    }

    return 0;
}

// $upload$/home/asim/index.c$