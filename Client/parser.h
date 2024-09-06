#ifndef PARSER_H
#define PARSER_H
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
// $upload$filePath$

int isValidPath(const char *filePath)
{
    struct stat buffer;
    if (stat(filePath, &buffer) == 0)
    {
        return S_ISREG(buffer.st_mode);
    }
    return 0;
}

void PARSER(const char *command, char *parsedCommand)
{
    char tempStr[256];
    strncpy(tempStr, command, sizeof(tempStr));
    tempStr[sizeof(tempStr) - 1] = '\0';
    
    char *commandType = strtok(tempStr, "$");

    if (strcmp(commandType, "upload") == 0)
    {

        char *path = strtok(NULL, "$");
        if (isValidPath(path))
        {
            sprintf(parsedCommand, "{\"command\": \"%s\", \"path\": \"%s\"}", commandType, path);
            return;
        }
        else
            strcpy(parsedCommand, "{\"error\":\"Invalid Path\"}");
            // strdup ..
        return;
    }
    else if (strcmp(commandType, "download") == 0)
    {
        char *path = strtok(NULL, "$");
        if (isValidPath(path))
        {
            sprintf(parsedCommand, "{\"command\": \"%s\", \"path\": \"%s\"}", commandType, path);
            return;
        }
        else
            strcpy(parsedCommand, "{\"error\":\"Invalid Path\"}");
        return;
    }
    else if(strcmp(commandType, "close") == 0){
            sprintf(parsedCommand, "{\"command\": \"%s\"}", commandType);
            return;
    }
    else
    {
        strcpy(parsedCommand, "{\"error\":\"Invalid Command\"}");
    }
}

#endif