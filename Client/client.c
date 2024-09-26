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
#include "runLength.h"

struct download_status
{
    int isDownloading;
    char *filename;
    int fileSize;
    int total_recieved;
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



void send_file(int clinetSocket, char *filepath)
{
    char *encoded_file = "./Client/encoded.txt";
    encodeFile(filepath,encoded_file);
    // send the filepath to the encoder
    FILE *file = fopen(encoded_file, "rb");
    char stream[1024];
    while (fgets(stream, 1024, file) != NULL)
    {
        send(clinetSocket, stream, strlen(stream), 0);
    }
    fclose(file);
    printf("File Uploaded Successfully.\n");
    
    //deleting the temp encoded file
    if (remove(encoded_file) == 0) {
        printf("Temp encoded File deleted successfully.\n");
    } else {
        perror("Error deleting Temp encoded file");
    }
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
    else if(status->valuestring, "failed")
    {
        printf("uploading this file %s will result in exceeding size limit\n",path->valuestring);
    }
}

void recieve_file(char *content, struct download_status *DN, int bufferBytes)
{
    FILE *file = fopen(DN->filename, "ab");
    if (file && DN->total_recieved < DN->fileSize)
    {
        printf("%i, %i, %i\n",DN->fileSize,DN->total_recieved, bufferBytes);

        int remaining = DN->fileSize - DN->total_recieved;
        int bytes_to_write = (remaining < bufferBytes) ? remaining : bufferBytes;

        fwrite(content, 1, bytes_to_write, file);

        DN->total_recieved += bytes_to_write;
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
    cJSON *Filesize = cJSON_GetObjectItem(ServerResponse, "filesize");
    if (status && strcmp(status->valuestring, "failed") == 0)
    {
        printf("Filed error from server side\n");
    }
    else if (strcmp(status->valuestring, "fetch") == 0)
    {
        printf("%s\n",cJSON_Print(ServerResponse));
        downloading->isDownloading = 1;
        downloading->filename = Filename->valuestring;
        downloading->fileSize = atoi(Filesize->valuestring);
    }
}

void handle_view(int clientSocket, cJSON *ServerResponse)
{
    printf("%s",cJSON_Print(ServerResponse));
}

int main()
{
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];

    struct download_status downloadingFile = {0, NULL, 0, 0};
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
        char serverResponse[4086];
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
                printf("%s",parsedCommand);
                printf("Error in command\n");
                continue;
            }
            send(client_socket, parsedCommand, strlen(parsedCommand), 0);
        }
        int bytesRecievedFromServer = recv(client_socket, serverResponse, sizeof(serverResponse), 0);
        // printf("Server Response: %s\n", serverResponse);
        if (downloadingFile.isDownloading)
        {
            recieve_file(serverResponse, &downloadingFile, bytesRecievedFromServer);
            if (strstr(serverResponse, "{\"status\":\"success\"}"))
            {
                downloadingFile.isDownloading = 0;
                downloadingFile.fileSize = 0;
                downloadingFile.total_recieved = 0;
                char* temp_file = "temp.txt";
                decodeFile(downloadingFile.filename,temp_file);
                // now deleting the encoded file
                char* decoy_name = "__.txt";
                if (rename(downloadingFile.filename,decoy_name))
                {
                    printf("File name changed 1\n");
                    if(rename(temp_file,downloadingFile.filename))
                    {
                        printf("Second File name changed successfully 2 going towards deletion\n");
                        if (remove(decoy_name) == 0) {
                            printf("Temp encoded File deleted successfully.\n");
                        } else {
                            perror("Error deleting Temp encoded file");
                        }
                    }   
                }
                else{
                    printf("Failed to change 1\n");
                }

                continue;
            }
            continue;
        }

        cJSON *ServerResponseJSON = cJSON_Parse(serverResponse);
        cJSON *serverCommand = cJSON_GetObjectItem(ServerResponseJSON, "command");
        if (serverCommand && strcmp(serverCommand->valuestring, "upload") == 0)
            handel_upload(client_socket, ServerResponseJSON, parsedJsonCommand);
        else if (serverCommand && strcmp(serverCommand->valuestring, "download") == 0)
        {
            handel_download(client_socket, ServerResponseJSON, &downloadingFile);
        }
        else if(serverCommand && strcmp(serverCommand->valuestring, "view") == 0)
        {
            printf("Server Response: %s\n", serverResponse);
            handle_view(client_socket,ServerResponseJSON);
        }
    }
    printf("Client Socket got disconnected due to error in Command Passed\n");
    close(client_socket);
    return 0;
}
