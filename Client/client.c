#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "parser.h"
#include <cjson/cJSON.h>
#include <string.h>

struct download_status
{
    int isDownloading;
    char *filename;
    int fileSize;
};

char *extract_filename(char *path)
{

    char *last_slash = strrchr(path, '/');

    if (last_slash == NULL)
    {
        return path;
    }

    return last_slash + 1;
}

int getFileSize(char *path)
{
    struct stat fileInfo;
    stat(path, &fileInfo);
    return fileInfo.st_size;
}

void send_file(int clinetSocket, char *filepath)
{
    char stream[1024];
    FILE *file = fopen(filepath, "rb");

    while (fgets(stream, 1024, file) != NULL)
    {
        send(clinetSocket, stream, strlen(stream), 0);
    }

    fclose(file);
    printf("File Uploaded Successfully.\n");
    return;
}

void handel_upload(int clientSocket, cJSON *ServerResponse, cJSON *Command)
{
    cJSON *status = cJSON_GetObjectItem(ServerResponse, "status");
    cJSON *command = cJSON_GetObjectItem(Command, "command");
    cJSON *path = cJSON_GetObjectItem(Command, "path");

    if (strcmp(status->valuestring, "ready") == 0)
    {
        char *Msg = malloc(256);
        sprintf(Msg, "{\"command\":\"upload\",\"status\":\"incoming\", \"filename\":\"%s\",\"filesize\":\"%i\"}", extract_filename(path->valuestring), getFileSize(path->valuestring));
        send(clientSocket, Msg, strlen(Msg), 0);
        printf("Please wait while file is being uploded ...\n");

        send_file(clientSocket, path->valuestring);

        Msg = "{\"status\":\"success\"}";
        printf("File uploaded Successfully\n");
        send(clientSocket, Msg, strlen(Msg), 0);
    }
}

void recieve_file(char *content, char *filename)
{
    FILE *file = fopen(filename, "ab");
    if (file)
    {
        fprintf(file, "%s", content);
        fclose(file);
    }
    else
    {
        perror("Error opening file for writing");
    }
}

void handel_download(int clientSocket, cJSON *ServerResponse, struct download_status *downloading)
{
    cJSON *status = cJSON_GetObjectItem(ServerResponse, "status");
    cJSON *Filename = cJSON_GetObjectItem(ServerResponse, "filename");

    if (status && strcmp(status->valuestring, "failed") == 0)
    {
        printf("Filed error from server side\n");
    }
    else if (strcmp(status->valuestring, "fetch") == 0)
    {
        downloading->isDownloading = 1;
        downloading->filename = Filename->valuestring;
    }
}
int main()
{
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];

    struct download_status downloadingFile = {0, NULL, 0};
    printf("%i\n", downloadingFile.isDownloading);
    // Create a
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(3001);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        char serverResponse[1024];
        char command[256];
        char *parsedCommand = NULL;
        cJSON *parsedJsonCommand = NULL;
        if (!downloadingFile.isDownloading)
        {
            printf("Enter Command: ");
            fgets(command, sizeof(command), stdin);
            parsedCommand = malloc(256);
            PARSER(command, parsedCommand);

            parsedJsonCommand = cJSON_Parse(parsedCommand);
            cJSON *commandType = cJSON_GetObjectItem(parsedJsonCommand, "command");
            cJSON *error = cJSON_GetObjectItem(parsedJsonCommand, "error");
            cJSON *path = cJSON_GetObjectItem(parsedJsonCommand, "path");
            if (error)
            {
                printf("Error in command\n");
                continue;
            }
            send(client_socket, parsedCommand, strlen(parsedCommand), 0);
        }
        int bytesRecievedFromServer = recv(client_socket, serverResponse, sizeof(serverResponse), 0);
        // printf("Server Response: %s\n", serverResponse);
        if(downloadingFile.isDownloading)
        {
            recieve_file(serverResponse, downloadingFile.filename);
            continue;
        }
        
        cJSON *ServerResponseJSON = cJSON_Parse(serverResponse);
        cJSON *serverCommand = cJSON_GetObjectItem(ServerResponseJSON, "command");
        if(serverCommand && strcmp(serverCommand->valuestring, "upload")==0)
            handel_upload(client_socket,ServerResponseJSON,parsedJsonCommand);
        else if(serverCommand && strcmp(serverCommand->valuestring, "download")==0)
        {
            handel_download(client_socket,ServerResponseJSON,&downloadingFile);
        }
    }
    printf("Client Socket got disconnected due to error in Command Passed\n");
    close(client_socket);
    return 0;
}
