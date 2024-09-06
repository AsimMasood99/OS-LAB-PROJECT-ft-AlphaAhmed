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
char *extract_filename(char *path)
{

    char *last_slash = strrchr(path, '/');

    if (last_slash == NULL)
    {
        return path;
    }

    return last_slash + 1;
}

void processServerResponse(int clinetSocket, char *response, cJSON *commandJson)
{
    cJSON *responseJson = cJSON_Parse(response);
    cJSON *status = cJSON_GetObjectItem(responseJson, "status");
    cJSON *command = cJSON_GetObjectItem(responseJson, "command");

    if (strcmp(command->valuestring, "upload") == 0)
    {   
        printf("hi\n");
        cJSON *path = cJSON_GetObjectItem(commandJson, "path");
        if (strcmp(status->valuestring, "ready") == 0)
        {
            int bytesRead = 0;
            char stream[1024];
            FILE *file = fopen(path->valuestring, "rb");
            char *Msg = malloc(256);
            sprintf(Msg, "{\"command\":\"upload\",\"status\":\"incoming\", \"filename\":\"%s\"}", extract_filename(path->valuestring));

            printf("killing command:%s", Msg);
            send(clinetSocket, Msg, strlen(Msg), 0);
            printf("Please wait while file is being uploded ...");
            // while ((bytesRead = fread(stream, 1, sizeof(stream), file)) > 0)
            // {
            //     printf("%s\n",stream);
            //     send(clinetSocket, stream, bytesRead, 0);
            // }

            while(fgets(stream,1024,file)!=NULL){
                printf("%s",stream);
                send(clinetSocket,stream,strlen(stream),0);
            }

            // add an end of file dilimiter here as such the Msg below doesnt gets cut in 2 strings on server side .. on server i have same size buffer
            
            Msg = "{\"status\":\"success\"}";
            
            
            
            printf("File uploaded Successfully\n");
            send(clinetSocket, Msg, strlen(Msg), 0);
        }
    }
}

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
        printf("Enter Command: ");
        fgets(command, sizeof(command), stdin);

        char *parsedCommand = malloc(256);
        PARSER(command, parsedCommand);

        // struct json_object *parsed_obj = json_tokener_parse(parsedCommand);
        // struct json_object

        cJSON *parsedJsonCommand = cJSON_Parse(parsedCommand);
        cJSON *error = cJSON_GetObjectItem(parsedJsonCommand, "error");

        if (error)
        {
            printf("Error in command");
        }
        else
        {
            send(client_socket, parsedCommand, strlen(parsedCommand), 0);
        }
        int bytesRecievedFromServer = recv(client_socket, serverResponse, sizeof(serverResponse), 0);
        printf("Server Response: %s\n", serverResponse);
        processServerResponse(client_socket, serverResponse, parsedJsonCommand);
    }
    close(client_socket);
    return 0;
}
// memcopy instead of strcpy