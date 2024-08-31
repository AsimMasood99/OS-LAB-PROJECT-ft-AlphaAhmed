#ifndef PARSER_H
#define PARSER_H
#include <string.h>
#include <stdio.h>
#include<sys/stat.h>
// $upload$filePath$

int isValidPath(const char* filePath){
    // FILE *file = fopen(filePath, "r");
    // if(file)
    //     return 1;
    // return 0;
      struct stat buffer;
    if (stat(filePath, &buffer) == 0) {
        return S_ISREG(buffer.st_mode); // Check if it is a regular file
    }
    return 0; 

}

void PARSER(const char* command, char* parsedCommand){
    char tempStr[256];
    strcpy(tempStr, command);
    tempStr[sizeof(tempStr)-1] = '\0';
    char* commandType = strtok(tempStr, "$");

    if(strcmp(commandType, "upload") == 0){
    
        char* path = strtok(NULL,"$");
        if(isValidPath(path)){
            sprintf(parsedCommand, "{\"command\": \"%s\", \"path\": \"%s\"}", commandType, path);
            return;
        }
        else 
            strcpy(parsedCommand, "{\"error\":\"Invalid Path\"}");
            return;
    }
    else if(strcmp(commandType, "download") == 0){
        char* path = strtok(NULL,"$");
        if(isValidPath(path)){
            sprintf(parsedCommand, "{\"command\": \"%s\", \"path\": \"%s\"}", commandType, path);
            return;
        }
        else 
            strcpy(parsedCommand, "{\"error\":\"Invalid Path\"}");
            return;
    }
    else {
        strcpy(parsedCommand, "{\"error\":\"Invalid Command\"}");
    }
    
}



#endif 