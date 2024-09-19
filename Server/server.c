#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <sys/stat.h>
#include <pthread.h>

#define NUM_THREADS 10


struct Recieving_File
{
    int receiving;
    char *filename;
    int fileSize;
};

struct thread_info {
    int thread_id;
    int is_running;
    char buffer[1024] ;
    int file_bytes ;
    int new_socket ;
    struct Recieving_File receivingFile;
    int server_socket ;
    struct sockaddr_in client_addr;
};

 pthread_t threads[NUM_THREADS];
 struct thread_info threadInfos[NUM_THREADS];



int isSpaceAvailable(const char *path, int maxSpace)
{
    return 1;
}

int getFileSize(char *path)
{
    struct stat fileInfo;
    stat(path, &fileInfo);
    return fileInfo.st_size;
}

void recieve_file(char *content, char *filename, int filesize, int *totalRecievedBytes, int bufferBytes)
{
    FILE *file = fopen(filename, "ab");
    if (!file)
    {
        perror("Error in openinig file: ");
    }
    if (file && *totalRecievedBytes < filesize)
    {
        fwrite(content, 1, filesize - *totalRecievedBytes > 1023 ? 1023 : filesize - *totalRecievedBytes - 1, file);
        *totalRecievedBytes += bufferBytes;
        fclose(file);
    }
}

void handel_upload(int socket, cJSON *command, struct Recieving_File *recievingStatus, char *ip)
{
    char folder_name[256] = "./Server/Storage/";
    strcat(folder_name, ip);

    cJSON *status = cJSON_GetObjectItem(command, "status");
    if (status && strcmp(status->valuestring, "incoming") == 0)
    {
        recievingStatus->receiving = 1;
        recievingStatus->filename = malloc(strlen(folder_name) + strlen(cJSON_GetObjectItem(command, "filename")->valuestring) + 2);
        sprintf(recievingStatus->filename, "%s/%s", folder_name, cJSON_GetObjectItem(command, "filename")->valuestring);
        recievingStatus->fileSize = atoi(cJSON_GetObjectItem(command, "filesize")->valuestring);
        return;
    }
    struct stat folder = {0};
    // if there is a folder
    if (stat(folder_name, &folder) == -1)
    {
        mkdir(folder_name, 0700);
    }
    char *response = "{\"status\":\"ready\", \"command\":\"upload\"}";
    send(socket, response, strlen(response), 0);
    return;
}

void send_file(int socket, char *filepath)
{
    char stream[1024];
    FILE *file = fopen(filepath, "rb");

    while (fgets(stream, 1024, file) != NULL)
    {
        send(socket, stream, strlen(stream), 0);
    }

    fclose(file);
    return;
}

void handel_download(int socket, cJSON *command, char *ip)
{
    cJSON *fileToCheck = cJSON_GetObjectItem(command, "filename"); // jahaz ider filename ana tha
    char folder_and_file_name[1024];
    snprintf(folder_and_file_name, sizeof(folder_and_file_name), "./Server/Storage/%s/%s", ip, fileToCheck->valuestring);

    if (access(folder_and_file_name, F_OK) == 0)
    {
        char download_response[256];
        sprintf(download_response, "{\"status\":\"fetch\",\"command\":\"download\",\"filename\":\"%s\"}", fileToCheck->valuestring);
        send(socket, download_response, strlen(download_response), 0);

        send_file(socket, folder_and_file_name);
        char *completion_Msg = "{\"status\":\"success\"}";
        printf("File uploaded Successfully\n");

        send(socket, completion_Msg, strlen(completion_Msg), 0);

        return;
    }
    else
    {
        printf("File %s is not available on server\n", fileToCheck->valuestring);
        char *failed_responce = "{\"status\":\"failed\"}";
        send(socket, failed_responce, strlen(failed_responce), 0);
    }
}







void client_handler_function(void * arg)
{
    //char buffer[1024] , int file_bytes  ,int new_socket ,struct Recieving_File receivingFile , int server_socket ,  struct sockaddr_in client_addr

   struct thread_info* info = (struct thread_info*)arg;
   
  while (1)   
        {
            memset(info->buffer, 0, sizeof(info->buffer));
            int bytes_received = 1;
            
            bytes_received = recv(info->new_socket, info->buffer, sizeof(info->buffer) - 1, 0);
            
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
                close(info->new_socket);
                info->is_running=0;
                break;                                       //return 
            }

            info->buffer[bytes_received] = '\0';

            if (info->receivingFile.receiving)
            {
                printf("reviing: %s\n", info->buffer);
            
                recieve_file(info->buffer, info->receivingFile.filename, info->receivingFile.fileSize, &info->file_bytes, bytes_received);
                
                if (strstr(info->buffer, "{\"status\":\"success\"}"))
                {
                    info->receivingFile.receiving = 0;
                    info->receivingFile.fileSize = 0;
                    info->file_bytes = 0;
                    printf("Closing\n");
                    continue;
                }
            } // this else if is for sending file to the client
            else
            {
                cJSON *jsonCommand = cJSON_Parse(info->buffer);
                if (jsonCommand == NULL)
                {
                    printf("Invalid JSON received\n");
                    continue;
                }

                cJSON *commandType = cJSON_GetObjectItem(jsonCommand, "command");
                if (commandType && strcmp(commandType->valuestring, "upload") == 0)
                {
                    handel_upload(info->new_socket, jsonCommand, &info->receivingFile, inet_ntoa(info->client_addr.sin_addr));
                }
                else if (commandType && strcmp(commandType->valuestring, "download") == 0)
                {
                    handel_download(info->new_socket, jsonCommand, inet_ntoa(info->client_addr.sin_addr));
                }
                else if (commandType && strcmp(commandType->valuestring, "close") == 0)
                {
                    close(info->new_socket);
                    close(info->server_socket);
                    info->is_running=0;
                    return;
                }

                cJSON_Delete(jsonCommand);
            }
        }

}

int main()
{
    while(1)
    {
        int server_socket, new_socket, client_addr_len;
        struct Recieving_File receivingFile = {0, NULL, 0};
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
            int file_bytes = 0;

            /*
            pthread_t threads[NUM_THREADS];
            struct thread_info threadInfos[NUM_THREADS];
            */
            int found=0;
            int threadId=0;
            for (size_t i = 0; i < NUM_THREADS; i++)
            {
                if(threadInfos[i].is_running!=1)
                {
                    //free thread so using it init
                    found=1;
                    threadId=i;
                    threadInfos[i].thread_id=i;

                    threadInfos[i].client_addr=client_addr;
                    threadInfos[i].file_bytes=file_bytes;
                    threadInfos[i].new_socket=new_socket;
                    threadInfos[i].server_socket=server_socket;

                    for (size_t i = 0; i < 1024; i++)
                    {
                        threadInfos[i].buffer[i]=buffer[i];
                    }
                    break;
                }
            }
            
            if (found==1 && pthread_create(&threads[threadId], NULL, client_handler_function, &threadInfos[threadId]) != 0)
            {
                perror("Error creating thread");
                return 1;
            }
                
            
        for (int i = 0; i < NUM_THREADS; i++) 
        {
            if(threadInfos[i].is_running==1)
                printf("Thread running No. %d",i);
                else 
                    printf("Thread empty No. %d",i);
                
            }
        
        }
    }

    return 0;
}