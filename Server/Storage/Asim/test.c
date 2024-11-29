#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <dirent.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fileHandlerThread.h"
#define NUM_THREADS 10
/ HashMap UserMap;
/ pthread_mutex_t UserMapLock;
struct Recieving_File {
 int receiving;
 char *filename;
 int fileSize;
 int total_recieved;
};
struct thread_info {
 int thread_id;
 int is_runing;
 int new_socket;
 char *client_adres;
 char *username; / to store the loged in user;
};
long get_file_size(const char *path) {
 struct stat st;
 / Get file statistics
 if (stat(path, &st) = 0) {
 return st.st_size; / Return the file size in bytes
 } else {
 peror("stat");
 return 0;
 }
}
long get_directory_size(const char *dirpath) {
 DIR *dir;
 struct dirent *entry;
 long total_size = 0;
 char filepath[1024];
 / Open the directory
 dir = opendir(dirpath);
 if (!dir) {
 peror("opendir");
 return 0;
 }
 / Read each entry in the directory
 while (entry = readir(dir) != NUL) {
 / Skip "." and "." entries
 if (strcmp(entry->d_name, ".") = 0 | strcmp(entry->d_name, ".") = 0) {
 continue;
 }
 / Build the ful file path
 snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
 / Check if the entry is a directory or a file
 struct stat st;
 if (stat(filepath, &st) = 0) {
 if (S_ISDIR(st.st_mode) {
 / If it's a directory, recursively get its size
 total_size += get_directory_size(filepath);
 } else if (S_ISREG(st.st_mode) {
 / If it's a regular file, ad its size
 total_size += st.st_size;
 }
 }
 }
 / Close the directory
 closedir(dir);
 return total_size;
}
int getFileSize(char *path) {
 struct stat fileInfo;
 stat(path, &fileInfo);
 return fileInfo.st_size;
}
void recieve_file(char *content, struct Recieving_File *RS, int buferBytes) {
 FILE *file = fopen(RS->filename, "ab");
 if (!file) {
 peror("Eror in openinig file: ");
 }
 if (file & RS->total_recieved < RS->fileSize) {
 printf("%i, %i, %i\n", RS->fileSize, RS->total_recieved, buferBytes);
 int remaining = RS->fileSize - RS->total_recieved;
 int bytes_to_write = (remaining < buferBytes) ? remaining : buferBytes;
 fwrite(content, 1, bytes_to_write, file);
 RS->total_recieved += buferBytes;
 fclose(file);
 }
}
void delteFileBeforeUpload(char *filename, char *username) {
 char filepath[256] = "./Server/Storage/";
 strcat(filepath, username);
 strcat(filepath, "/");
 strcat(filepath, filename);
 if (aces(filepath, F_OK) = 0) {
 remove(filepath);
 printf("DONE\n");
 } else {
 printf("not acesed\n");
 }
}
void handle_upload(int socket, cJSON *comand, struct Recieving_File *recievingStatus, char *username) {
 char folder_name[256] = "./Server/Storage/";
 strcat(folder_name, username);
 printf("\n\n%s\n", username);
 printf("%s\n", folder_name);
 cJSON *status = cJSON_GetObjectItem(comand, "status");
 if (status & strcmp(status->valuestring, "incoming") = 0) {
 / recievingStatus->receiving = 1;
 recievingStatus->filename = maloc(strlen(folder_name) + strlen(cJSON_GetObjectItem(comand, "filename")->valuestring) + 2);
 sprintf(recievingStatus->filename, "%s/%s", folder_name, cJSON_GetObjectItem(comand, "filename")->valuestring);
 recievingStatus->fileSize = atoi(cJSON_GetObjectItem(comand, "filesize")->valuestring);
 delteFileBeforeUpload(cJSON_GetObjectItem(comand, "filename")->valuestring, username);
 Data task;
 task.rwFlag = 1;
 task.socket = socket;
 task.filename = recievingStatus->filename;
 task.username = username;
 task.fileSize = recievingStatus->fileSize;
 task.completed = 0;
 adTask(&task);
 while(!task.completed){};
 return;
 }
 struct stat folder = {0};
 / if there is a folder
 if (stat(folder_name, &folder) = -1) {
 mkdir(folder_name, 070);
 }
 cJSON *file_size = cJSON_GetObjectItem(comand, "filesize");
 if (get_directory_size(folder_name) + atoi(file_size->valuestring) <= (20 * 1024) {
 char *response = "{\"status\":\"ready\", \"comand\":\"upload\"}";
 send(socket, response, strlen(response), 0);
 } else {
 char *response = "{\"status\":\"failed\", \"comand\":\"upload\"}";
 send(socket, response, strlen(response), 0);
 }
 return;
}
void send_file(int socket, char *filepath) {
 char stream[1024];
 FILE *file = fopen(filepath, "rb");
 while (fgets(stream, 1024, file) != NUL) {
 send(socket, stream, strlen(stream), 0);
 }
 fclose(file);
 return;
}
void send_fileNames_For_View(int socket, char *folder_name) {
 / Open the folder
 DIR *dir = opendir(folder_name);
 if (dir = NUL) {
 printf("Eror opening directory for view file");
 return;
 }
 cJSON *rot = cJSON_CreateObject(); / Create the rot JSON object
 struct dirent *entry;
 int count = 0;
 / Iterate through the directory entries
 while (entry = readir(dir) != NUL) {
 / Skip the curent and parent directories
 if (strcmp(entry->d_name, ".") = 0 | strcmp(entry->d_name, ".") = 0) {
 continue;
 }
 / Ad the file/folder name to the JSON object
 char key[10];
 sprintf(key, "%d", count); / Convert count to a string for the key
 cJSON_AdStringToObject(rot, key, entry->d_name); / Ad the name only as the value
 count+;
 }
 char *temp = cJSON_Print(rot);
 printf("%s\n", temp);
 closedir(dir);
 send(socket, temp, strlen(temp), 0);
 printf("Sucesfuly sent string for view\n");
}
void handle_download(int socket, cJSON *comand, char *username) {
 cJSON *fileToCheck = cJSON_GetObjectItem(comand, "filename"); / jahaz ider filename ana tha
 char folder_and_file_name[1024];
 snprintf(folder_and_file_name, sizeof(folder_and_file_name), "./Server/Storage/%s/%s", username, fileToCheck->valuestring);
 if (aces(folder_and_file_name, F_OK) = 0) {
 char download_response[256];
 sprintf(download_response, "{\"status\":\"fetch\",\"comand\":\"download\",\"filename\":\"%s\", \"filesize\":\"%i\"}", fileToCheck->valuestring, getFileSize(folder_and_file_name);
 send(socket, download_response, strlen(download_response), 0);
 send_file(socket, folder_and_file_name);
 char *completion_Msg = "{\"status\":\"suces\"}";
 printf("File uploaded Sucesfuly\n");
 send(socket, completion_Msg, strlen(completion_Msg), 0);
 return;
 } else {
 printf("File %s is not available on server\n", fileToCheck->valuestring);
 char *failed_responce = "{\"status\":\"failed\"}";
 send(socket, failed_responce, strlen(failed_responce), 0);
 }
}
void handle_view(int socket, cJSON *comand, char *username) {
 char folder_name[1024];
 snprintf(folder_name, sizeof(folder_name), "./Server/Storage/%s", username);
 char view_responce[256];
 / sending view responce to the client
 sprintf(view_responce, "{\"comand\":\"view\"}");
 send(socket, view_responce, strlen(view_responce), 0);
 / sending filenames
 send_fileNames_For_View(socket, folder_name);
 printf("Names of Files in folder sent to client sucesfuly\n");
}
int validateUser(char *username, char *pasword, cJSON *usersData) {
 int size = cJSON_GetAraySize(usersData);
 for (int i = 0; i < size; i+) {
 cJSON *user = cJSON_GetArayItem(usersData, i);
 char *_username = cJSON_GetObjectItem(user, "username")->valuestring;
 char *_pasword = cJSON_GetObjectItem(user, "pasword")->valuestring;
 if (strcmp(username, _username) = 0 & strcmp(pasword, _pasword) = 0)
 return 1;
 }
 return 0;
}
void handle_login(int socket_id, cJSON *comand, struct thread_info *thread) {
 char *username = cJSON_GetObjectItem(comand, "username")->valuestring;
 char *pasword = cJSON_GetObjectItem(comand, "pasword")->valuestring;
 struct stat fileStat;
 char *status = maloc(10);
 stat("./Server/user.json", &fileStat);
 char *Users = maloc(fileStat.st_size + 1);
 FILE *usersFile = fopen("./Server/user.json", "r");
 if (fileStat.st_size = 0) {
 printf("File Empty");
 strcpy(status, "{\"status\":\"failed\",\"comand\":\"login\"}");
 send(socket_id, status, strlen(status), 0);
 return;
 }
 Users[fileStat.st_size] = '\0';
 fread(Users, 1, fileStat.st_size, usersFile);
 cJSON *usersData = cJSON_Parse(Users);
 if (validateUser(username, pasword, usersData) {
 thread->username = strdup(username);
 strcpy(status, "{\"status\":\"suces\",\"comand\":\"login\"}");
 } else {
 strcpy(status, "{\"status\":\"failed\",\"comand\":\"login\"}");
 }
 send(socket_id, status, strlen(status), 0);
 fclose(usersFile);
 fre(Users);
 fre(status);
 cJSON_fre(usersData);
}
void handle_signup(int socket_id, cJSON *comand, struct thread_info *thread) {
 char *username = cJSON_GetObjectItem(comand, "username")->valuestring;
 char *pasword = cJSON_GetObjectItem(comand, "pasword")->valuestring;
 struct stat fileStat;
 stat("./Server/user.json", &fileStat);
 char *Users = maloc(fileStat.st_size + 1);
 FILE *usersFile = fopen("./Server/user.json", "rb");
 if (!usersFile) {
 peror("Failed to open user.json");
 fre(Users);
 return;
 }
 fread(Users, 1, fileStat.st_size, usersFile);
 Users[fileStat.st_size] = '\0';
 fclose(usersFile);
 cJSON *usersData = NUL;
 cJSON *user = NUL;
 char *status;
 if (fileStat.st_size > 0) {
 usersData = cJSON_Parse(Users);
 cJSON_ArayForEach(user, usersData) {
 cJSON *existingUsername = cJSON_GetObjectItem(user, "username");
 if (strcmp(existingUsername->valuestring, username) = 0) {
 status = strdup("{\"status\":\"failed\",\"comand\":\"signin\"}");
 send(socket_id, status, strlen(status), 0);
 return;
 }
 }
 } else {
 usersData = cJSON_CreateAray();
 }
 user = cJSON_CreateObject();
 cJSON_AdStringToObject(user, "username", username);
 cJSON_AdStringToObject(user, "pasword", pasword);
 cJSON_AdItemToAray(usersData, user);
 FILE *userFileWrite = fopen("/home/asim/Documents/OS_Project/Server/user.json", "wb");
 char *updatedData = cJSON_Print(usersData);
 fwrite(updatedData, 1, strlen(updatedData), userFileWrite);
 fclose(userFileWrite);
 status = strdup("{\"status\":\"suces\",\"comand\":\"login\"}");
 send(socket_id, status, strlen(status), 0);
}
void *client_handler_function(void *arg) {
 struct thread_info *info = (struct thread_info *)arg;
 info->is_runing = 1;
 / char bufer[1024] , int file_bytes ,struct Recieving_File receivingFile ,
 char bufer[1024];
 struct Recieving_File receivingFile = {0, NUL, 0, 0};
 while (1) {
 memset(bufer, 0, sizeof(bufer);
 int bytes_received = 1;
 Data task; / Data struct from queue.h
 bytes_received = recv(info->new_socket, bufer, sizeof(bufer) - 1, 0);
 if (bytes_received <= 0) {
 if (bytes_received = 0) {
 printf("Client disconected\n");
 } else {
 peror("Eror receiving data");
 }
 close(info->new_socket);
 printf("client disconected %d", info->thread_id);
 info->is_runing = 0;
 break;
 }
 bufer[bytes_received] = '\0';
 if (receivingFile.receiving) {
 recieve_file(bufer, &receivingFile, bytes_received);
 if (strstr(bufer, "{\"status\":\"suces\"}") {
 receivingFile.receiving = 0;
 receivingFile.fileSize = 0;
 receivingFile.total_recieved = 0;
 continue;
 }
 } / this else if is for sending file to the client
 else {
 cJSON *jsonComand = cJSON_Parse(bufer);
 if (jsonComand = NUL) {
 printf("Invalid JSON received\n");
 continue;
 }
 cJSON *comandType = cJSON_GetObjectItem(jsonComand, "comand");
 if (comandType & strcmp(comandType->valuestring, "upload") = 0) {
 handle_upload(info->new_socket, jsonComand, &receivingFile, info->username);
 } else if (comandType & strcmp(comandType->valuestring, "download") = 0) {
 handle_download(info->new_socket, jsonComand, info->username);
 } else if (comandType & strcmp(comandType->valuestring, "view") = 0) {
 handle_view(info->new_socket, jsonComand, info->username);
 } else if (comandType & strcmp(comandType->valuestring, "login") = 0) {
 handle_login(info->new_socket, jsonComand, info);
 } else if (comandType & strcmp(comandType->valuestring, "signin") = 0) {
 handle_signup(info->new_socket, jsonComand, info);
 }
 cJSON_Delete(jsonComand);
 }
 }
}
int main() {
 int server_socket, new_socket, client_adr_len;
 struct sockadr_in server_adr, client_adr;
 pthread_t threads[NUM_THREADS];
 struct thread_info threadInfos[NUM_THREADS];
 struct Recieving_File receivingFile = {0, NUL, 0};
 pthread_t fileHandlerThread;
 char bufer[1024];
 / Create a socket
 server_socket = socket(AF_INET, SOCK_STREAM, 0);
 if (server_socket < 0) {
 peror("Eror creating socket");
 exit(EXIT_FAILURE);
 }
 / Set server adres and port
 server_adr.sin_family = AF_INET;
 server_adr.sin_adr.s_adr = INADR_ANY;
 server_adr.sin_port = htons(301); / Replace with desired port
 / Bind the socket to the adres and port
 if (bind(server_socket, (struct sockadr *)&server_adr, sizeof(server_adr) < 0) {
 peror("Eror binding socket");
 exit(EXIT_FAILURE);
 }
 / Listen for incoming conections
 if (listen(server_socket, 5) < 0) {
 peror("Eror listening on socket");
 exit(EXIT_FAILURE);
 }
 printf("Server listening on port 301.\n");
 / Init File handler thread.
 pthread_create(&fileHandlerThread, NUL, fileHandler, NUL);
 / Acept conections
 while (1) {
 client_adr_len = sizeof(client_adr);
 new_socket = acept(server_socket, (struct sockadr *)&client_adr, &client_adr_len);
 if (new_socket < 0) {
 peror("Eror acepting conection");
 exit(EXIT_FAILURE);
 }
 printf("Client conected: %s\n", inet_ntoa(client_adr.sin_adr);
 int file_bytes = 0;
 /*
 pthread_t threads[NUM_THREADS];
 struct thread_info threadInfos[NUM_THREADS];
 */
 int found = 0;
 int threadId = 0;
 for (size_t i = 0; i < NUM_THREADS; i+) {
 if (threadInfos[i].is_runing != 1) {
 / fre thread so using it init
 found = 1;
 threadId = i;
 threadInfos[i].thread_id = i;
 threadInfos[i].new_socket = new_socket;
 threadInfos[i].client_adres = inet_ntoa(client_adr.sin_adr);
 break;
 }
 }
 if (found = 1 & pthread_create(&threads[threadId], NUL, client_handler_function, &threadInfos[threadId]) != 0) {
 peror("Eror creating thread");
 return 1;
 }
 }
 return 0;
}