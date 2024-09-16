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

int printInFileFlag = 0; // this flag will become 1 when the server is sending file data to client
char *Filename;
int just_keep_printing = 0; // this bool is 1 when the server is sending the files data to client so the client will not take any commands
char *extract_filename(char *path)
{

    char *last_slash = strrchr(path, '/');

    if (last_slash == NULL)
    {
        return path;
    }

    return last_slash + 1;
}

// checking if the server is ready to receive file and then send file
void processServerResponse(int clinetSocket, char *response, cJSON *commandJson)
{
    cJSON *responseJson = cJSON_Parse(response);
    cJSON *status = cJSON_GetObjectItem(responseJson, "status");
    cJSON *command = cJSON_GetObjectItem(responseJson, "command");
    cJSON *_Filename = cJSON_GetObjectItem(responseJson, "filename");
    printf("\n Commad : %s\n", command->valuestring);
    if (strcmp(status->valuestring, "failed") == 0)
    {
        printf("Filed error from server side\n");
    }
    if (strcmp(command->valuestring, "upload") == 0)
    {
        printf("reaching upload if\n");
        printf("hi\n");
        cJSON *path = cJSON_GetObjectItem(commandJson, "path"); // /home/ahmad/filename.txt
        if (strcmp(status->valuestring, "ready") == 0)
        {
            int bytesRead = 0;
            char stream[1024];

            //"ab" is the file mode:
            // "a" (append): This mode opens the file in append mode. It creates the file if it doesn't exist, and if the file exists, it opens the file for writing at the end (appends to it).
            // "b" (binary mode): Since this is binary mode, it treats the file as a binary file, not a text file, so it doesn’t process newline characters or encoding transformations (like \r\n on Windows).
            FILE *file = fopen(path->valuestring, "rb");
            char *Msg = malloc(256);
            sprintf(Msg, "{\"command\":\"upload\",\"status\":\"incoming\", \"filename\":\"%s\"}", extract_filename(path->valuestring));

            printf("killing command:%s", Msg);
            send(clinetSocket, Msg, strlen(Msg), 0);
            printf("Please wait while file is being uploded ...\n");
            // while ((bytesRead = fread(stream, 1, sizeof(stream), file)) > 0)
            // {
            //     printf("%s\n",stream);
            //     send(clinetSocket, stream, bytesRead, 0);
            // }

            while (fgets(stream, 1024, file) != NULL)
            {
                printf("%s", stream);
                send(clinetSocket, stream, strlen(stream), 0);
            }

            // add an end of file dilimiter here as such the Msg below doesnt gets cut in 2 strings on server side .. on server i have same size buffer

            Msg = "{\"status\":\"success\"}";
            printf("File uploaded Successfully\n");
            send(clinetSocket, Msg, strlen(Msg), 0);
        }
    }
    else if (strcmp(command->valuestring, "download") == 0 || printInFileFlag == 1)
    {
        printf("Reached the download if\n");
        if (printInFileFlag == 1)
        {
            printf("Reached the print if\n");
            //+++++++++++++++++++++++++++++++++++++++++++++++++++
            // if (strcmp(status->valuestring, "success") == 0)
            // {
            //     printInFileFlag = 0;
            //     printf("Closing\n");
            // }
            if (strstr(response, "{\"status\":\"success\"}"))
            {
                printInFileFlag = 0;
                printf("Closing the downloaded file in client\n");
            }
            else
            {
                FILE *file = fopen(Filename, "rb");
                if (file)
                {
                    fprintf(file, "%s", response);
                    fclose(file);
                }
                else
                {
                    perror("Error opening file for writing");
                }
            }
        }
        // this is just to
        else if (strcmp(status->valuestring, "fetch") ==0 && printInFileFlag == 0)
        {
            // char *Msg = malloc(256);
            printf("Printing Flag turned on\n");
            printInFileFlag =1;
            Filename = _Filename->valuestring;
            printf("Client is ready to download data!!!");
            // sprintf(Msg, "{\"command\":\"download\",\"status\":\"ReadyToReceive");
            // send(clinetSocket,Msg,strlen(Msg),0);
        }
    }
    else
    {
        printf("Not going in any if :( in ProcessServerResponce Function\n");
    }
}

int main()
{
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];

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
        char *parsedCommand=NULL;
        cJSON *parsedJsonCommand=NULL;
        if (printInFileFlag == 0)
        {
            // getting command from user
            printf("Enter Command: ");
            fgets(command, sizeof(command), stdin);

            parsedCommand = malloc(256);
            PARSER(command, parsedCommand);

            // struct json_object *parsed_obj = json_tokener_parse(parsedCommand);
            // struct json_object

            // here we are parsing the returned json string and stored in json obj where we can excess its values
            parsedJsonCommand = cJSON_Parse(parsedCommand);

            cJSON *error = cJSON_GetObjectItem(parsedJsonCommand, "error");
            cJSON *path = cJSON_GetObjectItem(parsedJsonCommand, "path");
            if (error)
            {
                printf("Error in command\n");
                break;
            }
            else
            {
                // if command is download then server then we have to send
                send(client_socket, parsedCommand, strlen(parsedCommand), 0);
            }
        }
        int bytesRecievedFromServer = recv(client_socket, serverResponse, sizeof(serverResponse), 0);
        //printf("Server Response: %s\n", serverResponse);
        processServerResponse(client_socket, serverResponse, parsedJsonCommand);
    }
    printf("Client Socket got disconnected due to error in Command Passed\n");
    close(client_socket);
    return 0;
}
// memcopy instead of strcpy
