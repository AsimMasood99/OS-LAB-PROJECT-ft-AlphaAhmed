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

struct isSendingFile
{
    int isSending;
    char *filename;
};


void sendFileToClient()
{
    
}


int main()
{
    int server_socket, new_socket, client_addr_len;
    struct isRecievingFile receivingFile = {0, NULL};
    struct isSendingFile SendingFile = {0, NULL};
    struct sockaddr_in server_addr, client_addr;
    int sending_to_server=0;
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
            // here is where we are receiving data from client (both command and files)
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
            // if this check is on then the client is sending us file content and we are storing it
            if (receivingFile.receiving)
            {
                printf("reviing: %s\n", buffer);
                //:strstr() is a predefined function used for string matching
                // buffer is the main string to be examined.
                // {\"status\":\"success\"} is the sub-string to be searched in string.

                // here we are closing the file..
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
            } // this else if is for sending file to the client
            else if(SendingFile.isSending==1)
            {
                int bytesRead = 0;
                char stream[1024];
                char *send_filepath[255];
                // file path on the server about to be sent to client
                sprintf(send_filepath,"./Server/Storage/%s/%s",inet_ntoa(client_addr.sin_addr),SendingFile.filename);
                FILE *_file = fopen(send_filepath, "rb");

                while(fgets(stream,1024,_file)!=NULL){
                    printf("%s",stream);
                    send(server_socket,stream,strlen(stream),0);
                }
                // this will happen when all files will be send    
                char* completion_Msg = "{\"status\":\"success\"}";
                printf("File uploaded Successfully\n");
                SendingFile.isSending=0;
                send(server_socket, completion_Msg, strlen(completion_Msg), 0);
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
                // if command is upload then do this
                if (commandType && strcmp(commandType->valuestring, "upload") == 0)
                {
                    //making folder for the client
                    char folder_name[256] = "./Server/Storage/";
                    // changes IP of client from int to string and then concatinates clients IP address number (string) with folder name.
                    strcat(folder_name, inet_ntoa(client_addr.sin_addr));

                    struct stat folder = {0};
                    // if there is a folder
                    if (stat(folder_name, &folder) == -1)
                    {
                        mkdir(folder_name, 0700);
                    }
                    // this status means that file is ready and is coming
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
                } // this else if is for download command 
                else if(commandType && strcmp(commandType->valuestring, "download") == 0)
                {
                    cJSON *fileToCheck= cJSON_GetObjectItem(jsonCommand,"path");
                    char folder_and_file_name[256] = "./Server/Storage/";
                    // changes IP of client from int to string and then concatinates clients IP address number (string) with folder name.
                    strcat(folder_and_file_name, inet_ntoa(client_addr.sin_addr));
                    
                    // Appending the "client.txt" to the folder path
                    sprintf(folder_and_file_name, "%s/%s", folder_and_file_name, fileToCheck);

                    // Check if the file exists using access() with F_OK (File OK)
                    if(access(folder_and_file_name,F_OK)==0)
                    {
                        // char* download_response = "{\"status\":\"fetch\",\"command\":\"download\",\"filename\":\"%s\"}",fileToCheck;
                        // send(new_socket, download_response, strlen(download_response), 0);

                        char download_response[256];  // Allocate enough space for the formatted string
                        sprintf(download_response, "{\"status\":\"fetch\",\"command\":\"download\",\"filename\":\"%s\"}", fileToCheck);
                        // Now send the formatted response
                        send(new_socket, download_response, strlen(download_response), 0);
                        sendFileToClient();
                    }
                    else
                    {
                        printf("File %s is not available on server\n", fileToCheck);
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