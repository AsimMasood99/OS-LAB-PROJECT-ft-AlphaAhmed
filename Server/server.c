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

struct isRecievingFile
{
    int receiving;
    char *filename;
};

int main()
{
    int server_socket, new_socket, client_addr_len;
    struct isRecievingFile receivingFile = {0, NULL};
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

            int bytes_received = recv(new_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0)
            {
                if (bytes_received == 0)
                {
                    printf("Client disconnected\n");
                }
                else
                {
                    perror("Error receiving data");
                }
                close(new_socket);
                break;
            }

            buffer[bytes_received] = '\0';

            if (receivingFile.receiving)
            {
                printf("reviing: %s\n", buffer);

                if (strstr(buffer, "{\"status\":\"success\"}"))
                {
                    receivingFile.receiving = 0;
                    printf("Closing\n");
                    continue;
                }

                FILE *file = fopen(receivingFile.filename, "ab");
                if (file)
                {
                    // fwrite(buffer, 1, bytes_received, file);
                    fprintf(file, "%s", buffer);
                    fclose(file);
                }
                else
                {
                    perror("Error opening file for writing");
                }
            }
            else
            {
                cJSON *jsonCommand = cJSON_Parse(buffer);
                if (jsonCommand == NULL)
                {
                    printf("Invalid JSON received\n");
                    continue;
                }

                cJSON *commandType = cJSON_GetObjectItem(jsonCommand, "command");
                cJSON *status = cJSON_GetObjectItem(jsonCommand, "status");

                if (commandType && strcmp(commandType->valuestring, "upload") == 0)
                {
                    char folder_name[256] = "./Server/Storage/";
                    strcat(folder_name, inet_ntoa(client_addr.sin_addr));

                    struct stat folder = {0};
                    if (stat(folder_name, &folder) == -1)
                    {
                        mkdir(folder_name, 0700);
                    }

                    if (status && strcmp(status->valuestring, "incoming") == 0)
                    {
                        receivingFile.receiving = 1;

                        receivingFile.filename = cJSON_GetObjectItem(jsonCommand, "filename")->valuestring;
                        ;
                        continue;
                    }
                    else
                    {
                        char *response = "{\"status\":\"ready\", \"command\":\"upload\"}";
                        send(new_socket, response, strlen(response), 0);
                    }
                }
                else if (commandType && strcmp(commandType->valuestring, "close") == 0)
                {
                    close(new_socket);
                    close(server_socket);
                    return 0;
                }

                cJSON_Delete(jsonCommand);
            }
        }
    }

    return 0;
}