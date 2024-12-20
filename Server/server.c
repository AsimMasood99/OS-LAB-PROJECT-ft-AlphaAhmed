#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <dirent.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../malloc/malloc.h"
#include "fileHandlerThread.h"

#define NUM_THREADS 10

// HashMap UserMap;
// pthread_mutex_t UserMapLock;

struct Recieving_File {
    int receiving;
    char *filename;
    int fileSize;
    int total_recieved;
};

struct thread_info {
    int thread_id;
    int is_running;
    int new_socket;
    char *client_address;
    char *username;  // to store the logged in user;
};

long get_file_size(const char *path) {
    struct stat st;

    // Get file statistics
    if (stat(path, &st) == 0) {
        return st.st_size;  // Return the file size in bytes
    } else {
        perror("stat");
        return 0;
    }
}

long get_directory_size(const char *dirpath) {
    DIR *dir;
    struct dirent *entry;
    long total_size = 0;
    char filepath[1024];

    // Open the directory
    dir = opendir(dirpath);
    if (!dir) {
        perror("opendir");
        return 0;
    }

    // Read each entry in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build the full file path
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

        // Check if the entry is a directory or a file
        struct stat st;
        if (stat(filepath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // If it's a directory, recursively get its size
                total_size += get_directory_size(filepath);
            } else if (S_ISREG(st.st_mode)) {
                // If it's a regular file, add its size
                total_size += st.st_size;
            }
        }
    }

    // Close the directory
    closedir(dir);

    return total_size;
}

int getFileSize(char *path) {
    struct stat fileInfo;
    stat(path, &fileInfo);
    return fileInfo.st_size;
}

void recieve_file(char *content, struct Recieving_File *RS, int bufferBytes) {
    FILE *file = fopen(RS->filename, "ab");
    if (!file) {
        perror("Error in openinig file: ");
    }
    if (file && RS->total_recieved < RS->fileSize) {
        printf("%i, %i, %i\n", RS->fileSize, RS->total_recieved, bufferBytes);
        int remaining = RS->fileSize - RS->total_recieved;
        int bytes_to_write = (remaining < bufferBytes) ? remaining : bufferBytes;

        fwrite(content, 1, bytes_to_write, file);
        RS->total_recieved += bufferBytes;
        fclose(file);
    }
}

void delteFileBeforeUpload(char *filename, char *username) {
    char filepath[256] = "./Server/Storage/";
    strcat(filepath, username);
    strcat(filepath, "/");
    strcat(filepath, filename);
    if (access(filepath, F_OK) == 0) {
        remove(filepath);
        printf("DONE\n");
    } else {
        printf("not accessed\n");
    }
}

void handle_upload(int socket, cJSON *command, char *username) {
    char folder_name[256] = "./Server/Storage/";
    strcat(folder_name, username);

    // printf("\n\n%s\n", username);
    // printf("%s\n", folder_name);

    cJSON *status = cJSON_GetObjectItem(command, "status");

    if (status && strcmp(status->valuestring, "incoming") == 0) {
        int mindex = C_malloc(strlen(folder_name) + strlen(cJSON_GetObjectItem(command, "filename")->valuestring) + 2);
        char *filePath = (char *)(Blocks_Register->arr[mindex]) + sizeof(DynamicBlock);
        sprintf(filePath, "%s/%s", folder_name, cJSON_GetObjectItem(command, "filename")->valuestring);
        delteFileBeforeUpload(cJSON_GetObjectItem(command, "filename")->valuestring, username);
        Data task;
        task.rwFlag = 1;
        task.socket = socket;
        task.filename = filePath;
        task.username = username;
        task.fileSize = atoi(cJSON_GetObjectItem(command, "filesize")->valuestring);

        task.completed = 0;
        addTask(&task);

        while (task.completed == 0) {
        };
        printf("Operation Successful\n");
        // C_Free(mindex);
        return;
    }
    struct stat folder = {0};
    // if there is a folder
    if (stat(folder_name, &folder) == -1) {
        mkdir(folder_name, 0700);
    }
    cJSON *file_size = cJSON_GetObjectItem(command, "filesize");

    if (get_directory_size(folder_name) + atoi(file_size->valuestring) <= (20 * 1024)) {
        char *response = "{\"status\":\"ready\", \"command\":\"upload\"}";
        send(socket, response, strlen(response), 0);
    } else {
        char *response = "{\"status\":\"failed\", \"command\":\"upload\"}";
        send(socket, response, strlen(response), 0);
    }
    return;
}

void send_file(int socket, char *filepath) {
    char stream[1024];
    FILE *file = fopen(filepath, "rb");

    while (fgets(stream, 1024, file) != NULL) {
        send(socket, stream, strlen(stream), 0);
    }

    fclose(file);
    return;
}

void send_fileNames_For_View(int socket, char *folder_name) {
    // Open the folder
    DIR *dir = opendir(folder_name);
    if (dir == NULL) {
        printf("Error opening directory for view file");
        return;
    }

    cJSON *root = cJSON_CreateObject();  // Create the root JSON object
    struct dirent *entry;
    int count = 0;

    // Iterate through the directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip the current and parent directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Add the file/folder name to the JSON object
        char key[10];
        sprintf(key, "%d", count);                          // Convert count to a string for the key
        cJSON_AddStringToObject(root, key, entry->d_name);  // Add the name only as the value
        count++;
    }
    char *temp = cJSON_Print(root);
    printf("%s\n", temp);
    closedir(dir);
    send(socket, temp, strlen(temp), 0);
    printf("Successfully sent string for view\n");
}

void handle_download(int socket, cJSON *command, char *username) {
    cJSON *fileToCheck = cJSON_GetObjectItem(command, "filename");  // jahaz ider filename ana tha
    char folder_and_file_name[1024];
    snprintf(folder_and_file_name, sizeof(folder_and_file_name), "./Server/Storage/%s/%s", username, fileToCheck->valuestring);

    if (access(folder_and_file_name, F_OK) == 0) {
        char download_response[256];
        sprintf(download_response, "{\"status\":\"fetch\",\"command\":\"download\",\"filename\":\"%s\", \"filesize\":\"%i\"}", fileToCheck->valuestring, getFileSize(folder_and_file_name));
        send(socket, download_response, strlen(download_response), 0);

        Data task;
        task.completed = 0;
        task.filename = folder_and_file_name;
        task.fileSize = -1;
        task.rwFlag = 0;
        task.socket = socket;
        task.username = username;
        addTask(&task);
        while (task.completed == 0) {
        };
        // send_file(socket, folder_and_file_name);
        char *completion_Msg = "{\"status\":\"success\"}";
        printf("File uploaded Successfully\n");

        send(socket, completion_Msg, strlen(completion_Msg), 0);

        return;
    } else {
        printf("File %s is not available on server\n", fileToCheck->valuestring);
        char *failed_responce = "{\"status\":\"failed\"}";
        send(socket, failed_responce, strlen(failed_responce), 0);
    }
}

void handle_view(int socket, cJSON *command, char *username) {
    char folder_name[1024];
    snprintf(folder_name, sizeof(folder_name), "./Server/Storage/%s", username);
    char view_responce[256];
    // sending view responce to the client
    sprintf(view_responce, "{\"command\":\"view\"}");
    send(socket, view_responce, strlen(view_responce), 0);
    // sending filenames
    send_fileNames_For_View(socket, folder_name);
    printf("Names of Files in folder sent to client successfully\n");
}

int validateUser(char *username, char *password, cJSON *usersData) {
    int size = cJSON_GetArraySize(usersData);
    for (int i = 0; i < size; i++) {
        cJSON *user = cJSON_GetArrayItem(usersData, i);
        char *_username = cJSON_GetObjectItem(user, "username")->valuestring;
        char *_password = cJSON_GetObjectItem(user, "password")->valuestring;

        if (strcmp(username, _username) == 0 && strcmp(password, _password) == 0)
            return 1;
    }
    return 0;
}

void handle_login(int socket_id, cJSON *command, struct thread_info *thread) {
    char *username = cJSON_GetObjectItem(command, "username")->valuestring;
    char *password = cJSON_GetObjectItem(command, "password")->valuestring;

    struct stat fileStat;
    int mindx = C_malloc(100);
    char *status = (char *)(Blocks_Register->arr[mindx]) + sizeof(DynamicBlock);

    stat("./Server/user.json", &fileStat);
    int mindx2 = C_malloc(fileStat.st_size + 1);
    char *Users = (char *)(Blocks_Register->arr[mindx2]) + sizeof(DynamicBlock);
    FILE *usersFile = fopen("./Server/user.json", "r");
    if (fileStat.st_size == 0) {
        printf("File Empty");
        strcpy(status, "{\"status\":\"failed\",\"command\":\"login\"}");
        send(socket_id, status, strlen(status), 0);
        return;
    }
    Users[fileStat.st_size] = '\0';
    fread(Users, 1, fileStat.st_size, usersFile);

    cJSON *usersData = cJSON_Parse(Users);

    if (validateUser(username, password, usersData)) {
        thread->username = strdup(username);
        strcpy(status, "{\"status\":\"success\",\"command\":\"login\"}");
    } else {
        strcpy(status, "{\"status\":\"failed\",\"command\":\"login\"}");
    }

    send(socket_id, status, strlen(status), 0);
    fclose(usersFile);

    // C_Free(mindx);
    // C_Free(mindx2);
    cJSON_free(usersData);
}

void handle_signup(int socket_id, cJSON *command, struct thread_info *thread) {
    char *username = cJSON_GetObjectItem(command, "username")->valuestring;
    char *password = cJSON_GetObjectItem(command, "password")->valuestring;

    struct stat fileStat;

    stat("./Server/user.json", &fileStat);
    int mindx2 = C_malloc(fileStat.st_size + 1);
    char *Users = (char *)(Blocks_Register->arr[mindx2]) + sizeof(DynamicBlock);

    FILE *usersFile = fopen("./Server/user.json", "rb");
    if (!usersFile) {
        perror("Failed to open user.json");
        // C_Free(Users);
        return;
    }
    fread(Users, 1, fileStat.st_size, usersFile);
    Users[fileStat.st_size] = '\0';

    fclose(usersFile);
    cJSON *usersData = NULL;
    cJSON *user = NULL;
    // int mindx = C_malloc(100);
    char *status;

    if (fileStat.st_size > 0) {
        usersData = cJSON_Parse(Users);

        cJSON_ArrayForEach(user, usersData) {
            cJSON *existingUsername = cJSON_GetObjectItem(user, "username");
            if (strcmp(existingUsername->valuestring, username) == 0) {
                status = strdup("{\"status\":\"failed\",\"command\":\"signin\"}");
                send(socket_id, status, strlen(status), 0);
                return;
            }
        }
    } else {
        usersData = cJSON_CreateArray();
    }

    user = cJSON_CreateObject();
    cJSON_AddStringToObject(user, "username", username);
    cJSON_AddStringToObject(user, "password", password);

    cJSON_AddItemToArray(usersData, user);
    FILE *userFileWrite = fopen("./Server/user.json", "wb");

    char *updatedData = cJSON_Print(usersData);
    fwrite(updatedData, 1, strlen(updatedData), userFileWrite);
    fclose(userFileWrite);
    printf("hree\n");
    status = strdup("{\"status\":\"success\",\"command\":\"login\"}");
    send(socket_id, status, strlen(status), 0);

}

void *client_handler_function(void *arg) {
    struct thread_info *info = (struct thread_info *)arg;
    info->is_running = 1;
    char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = 1;
        Data task;  // Data struct from queue.h
        bytes_received = recv(info->new_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Client disconnected\n");
            } else {
                perror("Error receiving data");
            }

            close(info->new_socket);
            printf("client disconnected  %d", info->thread_id);
            info->is_running = 0;
            break;
        }

        buffer[bytes_received] = '\0';

        // if (receivingFile.receiving) {
        //     recieve_file(buffer, &receivingFile, bytes_received);

        //     if (strstr(buffer, "{\"status\":\"success\"}")) {
        //         receivingFile.receiving = 0;
        //         receivingFile.fileSize = 0;
        //         receivingFile.total_recieved = 0;
        //         continue;
        //     }
        // }  // this else if is for sending file to the client
        // else {
        cJSON *jsonCommand = cJSON_Parse(buffer);
        if (jsonCommand == NULL) {
            printf("Invalid JSON received\n");
            continue;
        }

        cJSON *commandType = cJSON_GetObjectItem(jsonCommand, "command");
        if (commandType && strcmp(commandType->valuestring, "upload") == 0) {
            handle_upload(info->new_socket, jsonCommand, info->username);
        } else if (commandType && strcmp(commandType->valuestring, "download") == 0) {
            handle_download(info->new_socket, jsonCommand, info->username);
        } else if (commandType && strcmp(commandType->valuestring, "view") == 0) {
            handle_view(info->new_socket, jsonCommand, info->username);
        } else if (commandType && strcmp(commandType->valuestring, "login") == 0) {
            handle_login(info->new_socket, jsonCommand, info);
        } else if (commandType && strcmp(commandType->valuestring, "signin") == 0) {
            handle_signup(info->new_socket, jsonCommand, info);
        }

        cJSON_Delete(jsonCommand);
        // }
    }
}

int main() {
    // Init malloc //
    Blocks_Register = (DynamicArray *)malloc(sizeof(DynamicArray));

    // initializing the global block register
    initArray(Blocks_Register, 1 * 1024);             // array to keep the poiters is 1kb
    allocateArena(Blocks_Register, 5 * 1024 * 1024);  // but actual arena is 5MB

    // init server //

    int server_socket, new_socket, client_addr_len;
    struct sockaddr_in server_addr, client_addr;

    pthread_t threads[NUM_THREADS];
    struct thread_info threadInfos[NUM_THREADS];

    struct Recieving_File receivingFile = {0, NULL, 0};

    pthread_t fileHandlerThread;

    char buffer[1024];

    // Create a socket

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(3001);  // Replace with desired port

    // Bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 3001...\n");

    // Init File handler thread..
    pthread_create(&fileHandlerThread, NULL, fileHandler, NULL);
    // Accept connections
    while (1) {
        client_addr_len = sizeof(client_addr);
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_socket < 0) {
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
        for (size_t i = 0; i < NUM_THREADS; i++) {
            if (threadInfos[i].is_running != 1) {
                // free thread so using it init
                found = 1;
                threadId = i;
                threadInfos[i].thread_id = i;
                threadInfos[i].new_socket = new_socket;
                threadInfos[i].client_address = inet_ntoa(client_addr.sin_addr);

                break;
            }
        }

        if (found == 1 && pthread_create(&threads[threadId], NULL, client_handler_function, &threadInfos[threadId]) != 0) {
            perror("Error creating thread");
            return 1;
        }
    }

    return 0;
}