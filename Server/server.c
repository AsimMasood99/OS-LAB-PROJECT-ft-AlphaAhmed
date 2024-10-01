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
#include <dirent.h>
#define NUM_THREADS 10

struct Recieving_File
{
    int receiving;
    char *filename;
    int fileSize;
    int total_recieved;
};

struct thread_info
{
    int thread_id;
    int is_running;
    int new_socket;
    char *client_address;
};

long get_file_size(const char *path)
{
    struct stat st;

    // Get file statistics
    if (stat(path, &st) == 0)
    {
        return st.st_size; // Return the file size in bytes
    }
    else
    {
        perror("stat");
        return 0;
    }
}

long get_directory_size(const char *dirpath)
{
    DIR *dir;
    struct dirent *entry;
    long total_size = 0;
    char filepath[1024];

    // Open the directory
    dir = opendir(dirpath);
    if (!dir)
    {
        perror("opendir");
        return 0;
    }

    // Read each entry in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Build the full file path
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

        // Check if the entry is a directory or a file
        struct stat st;
        if (stat(filepath, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                // If it's a directory, recursively get its size
                total_size += get_directory_size(filepath);
            }
            else if (S_ISREG(st.st_mode))
            {
                // If it's a regular file, add its size
                total_size += st.st_size;
            }
        }
    }

    // Close the directory
    closedir(dir);

    return total_size;
}

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

void recieve_file(char *content, struct Recieving_File *RS, int bufferBytes)
{
    FILE *file = fopen(RS->filename, "ab");
    if (!file)
    {
        perror("Error in openinig file: ");
    }
    if (file && RS->total_recieved < RS->fileSize)
    {
        printf("%i, %i, %i\n", RS->fileSize, RS->total_recieved, bufferBytes);
        int remaining = RS->fileSize - RS->total_recieved;
        int bytes_to_write = (remaining < bufferBytes) ? remaining : bufferBytes;

        fwrite(content, 1, bytes_to_write, file);
        RS->total_recieved += bufferBytes;
        fclose(file);
    }
}

void delteFileBeforeUpload(char *filename, char *ip)
{
    char filepath[256] = "./Server/Storage/";
    strcat(filepath, ip);
    strcat(filepath, "/");
    strcat(filepath, filename);
    if (access(filepath, F_OK) == 0)
    {
        remove(filepath);
        printf("DONE\n");
    }
    else
    {
        printf("not accessed\n");
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
        delteFileBeforeUpload(cJSON_GetObjectItem(command, "filename")->valuestring, ip);

        return;
    }
    struct stat folder = {0};
    // if there is a folder
    printf("%s\n", cJSON_Print(command));
    if (stat(folder_name, &folder) == -1)
    {
        mkdir(folder_name, 0700);
    }
    cJSON *file_size = cJSON_GetObjectItem(command, "filesize");

    if (get_directory_size(folder_name) + atoi(file_size->valuestring) <= (20 * 1024))
    {

        char *response = "{\"status\":\"ready\", \"command\":\"upload\"}";
        send(socket, response, strlen(response), 0);
    }
    else
    {
        char *response = "{\"status\":\"failed\", \"command\":\"upload\"}";
        send(socket, response, strlen(response), 0);
    }
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

void send_fileNames_For_View(int socket, char *folder_name)
{
    // Open the folder
    DIR *dir = opendir(folder_name);
    if (dir == NULL)
    {
        printf("Error opening directory for view file");
        return;
    }

    cJSON *root = cJSON_CreateObject(); // Create the root JSON object
    struct dirent *entry;
    int count = 0;

    // Iterate through the directory entries
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip the current and parent directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Add the file/folder name to the JSON object
        char key[10];
        sprintf(key, "%d", count);                         // Convert count to a string for the key
        cJSON_AddStringToObject(root, key, entry->d_name); // Add the name only as the value
        count++;
    }
    char *temp = cJSON_Print(root);
    printf("%s\n", temp);
    closedir(dir);
    send(socket, temp, strlen(temp), 0);
    printf("Successfully sent string for view\n");
}

void handel_download(int socket, cJSON *command, char *ip)
{
    cJSON *fileToCheck = cJSON_GetObjectItem(command, "filename"); // jahaz ider filename ana tha
    char folder_and_file_name[1024];
    snprintf(folder_and_file_name, sizeof(folder_and_file_name), "./Server/Storage/%s/%s", ip, fileToCheck->valuestring);

    if (access(folder_and_file_name, F_OK) == 0)
    {
        char download_response[256];
        sprintf(download_response, "{\"status\":\"fetch\",\"command\":\"download\",\"filename\":\"%s\", \"filesize\":\"%i\"}", fileToCheck->valuestring, getFileSize(folder_and_file_name));
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

void handle_view(int socket, cJSON *command, char *ip)
{
    char folder_name[1024];
    snprintf(folder_name, sizeof(folder_name), "./Server/Storage/%s", ip);
    char view_responce[256];
    // sending view responce to the client
    sprintf(view_responce, "{\"command\":\"view\"}");
    send(socket, view_responce, strlen(view_responce), 0);
    // sending filenames
    send_fileNames_For_View(socket, folder_name);
    printf("Names of Files in folder sent to client successfully\n");
}

void handel_login(cJSON *command)
{
    char *username = cJSON_GetObjectItem("username", command);
    char *password = cJSON_GetObjectItem("password", command);

    FILE *usersFIle = fopen("user.json", "rb");
    char users[2048];
    fseek()
}

void *client_handler_function(void *arg)
{
    struct thread_info *info = (struct thread_info *)arg;
    info->is_running = 1;
    // char buffer[1024] , int file_bytes ,struct Recieving_File receivingFile ,

    char buffer[1024];
    struct Recieving_File receivingFile = {0, NULL, 0, 0};

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = 1;

        bytes_received = recv(info->new_socket, buffer, sizeof(buffer) - 1, 0);

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
            printf("client disconnected  %d", info->thread_id);
            info->is_running = 0;
            break;
        }

        buffer[bytes_received] = '\0';

        if (receivingFile.receiving)
        {

            recieve_file(buffer, &receivingFile, bytes_received);

            if (strstr(buffer, "{\"status\":\"success\"}"))
            {
                receivingFile.receiving = 0;
                receivingFile.fileSize = 0;
                receivingFile.total_recieved = 0;
                continue;
            }
        } // this else if is for sending file to the client
        else
        {
            cJSON *jsonCommand = cJSON_Parse(buffer);
            if (jsonCommand == NULL)
            {
                printf("Invalid JSON received\n");
                continue;
            }

            cJSON *commandType = cJSON_GetObjectItem(jsonCommand, "command");
            if (commandType && strcmp(commandType->valuestring, "upload") == 0)
            {
                handel_upload(info->new_socket, jsonCommand, &receivingFile, info->client_address);
            }
            else if (commandType && strcmp(commandType->valuestring, "download") == 0)
            {
                handel_download(info->new_socket, jsonCommand, info->client_address);
            }
            else if (commandType && strcmp(commandType->valuestring, "view") == 0)
            {
                handle_view(info->new_socket, jsonCommand, info->client_address);
            }

            cJSON_Delete(jsonCommand);
        }
    }
}

int main()
{
    pthread_t threads[NUM_THREADS];
    struct thread_info threadInfos[NUM_THREADS];

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
        int found = 0;
        int threadId = 0;
        for (size_t i = 0; i < NUM_THREADS; i++)
        {
            if (threadInfos[i].is_running != 1)
            {
                // free thread so using it init
                found = 1;
                threadId = i;
                threadInfos[i].thread_id = i;
                threadInfos[i].new_socket = new_socket;
                threadInfos[i].client_address = inet_ntoa(client_addr.sin_addr);

                break;
            }
        }

        if (found == 1 && pthread_create(&threads[threadId], NULL, client_handler_function, &threadInfos[threadId]) != 0)
        {
            perror("Error creating thread");
            return 1;
        }

        for (int i = 0; i < NUM_THREADS; i++)
        {

            // int res = pthread_tryjoin_np(threads[i], NULL); but not needed logic is implemented
            if (threadInfos[i].is_running == 1)
                printf("Thread running No. %d", i);
            else
                printf("Thread empty No. %d", i);
        }
    }

    return 0;
}