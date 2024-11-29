// #include "hashmap.h"
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#include "queue.h"
#include "tasksMap.h"

#define MaxQueueSize 128

typedef struct
{
    int front;
    int tail;
    sem_t empty;
    sem_t full;
    pthread_mutex_t mutex;
    Data *queue[MaxQueueSize];  // Data Struct coming from queue.h
} JobQueue;

JobQueue tasks;

typedef struct
{
    Data Task;
    int suspended;
    pthread_t thread;
} Thread;

void addTask(Data *task) {
    sem_wait(&tasks.empty);
    pthread_mutex_lock(&tasks.mutex);
    // Create deep copy of Data Struct //
    tasks.queue[tasks.front] = task;
    // tasks.queue[tasks.front].filename = strdup(task->filename);
    // tasks.queue[tasks.front].username = strdup(task->username);
    // tasks.queuse[tasks.front].rwFlag = task->rwFlag;
    // tasks.queue[tasks.front].socket = task->socket;
    // tasks.queue[tasks.front].fileSize = task->fileSize;
    // tasks.queue[tasks.front].completed =
    tasks.front = (tasks.front + 1) % MaxQueueSize;
    printf("Task Scheduled \n");
    pthread_mutex_unlock(&tasks.mutex);

    sem_post(&tasks.full);
}

void *Read(void *args) {
    Data *dt = (Data *)args;
    printf("Read thread executed \n");
    char stream[1024];
    FILE *file = fopen(dt->filename, "rb");

    while (fgets(stream, 1024, file) != NULL) {
        send(dt->socket, stream, strlen(stream), 0);
    }

    fclose(file);
    dt->completed = 1;
}

void *Write(void *args) {
    printf("Write thread executed \n");
    int bufferSize = 1024;
    char *buffer = malloc(bufferSize);
    Data *data = (Data *)args;
    int bytes_received = 0;
    int total_recieved = 0;
    while (1) {
        memset(buffer, 0, bufferSize);
        bytes_received = recv(data->socket, buffer, bufferSize - 1, 0);
        buffer[bytes_received] = '\0';
        FILE *file = fopen(data->filename, "ab");
        if (!file) {
            perror("Error in openinig file: ");
        }
        if (file && total_recieved < data->fileSize) {
            int remaining = data->fileSize - total_recieved;
            int bytes_to_write = (remaining < bytes_received) ? remaining : bytes_received;

            fwrite(buffer, 1, bytes_to_write, file);
            total_recieved += bytes_received;
            fclose(file);
        }
        if (strstr(buffer, "{\"status\":\"success\"}")) {
            break;
        }
    }
    data->completed = 1;
    free(buffer);
}

void *fileHandler(void *args) {
    sem_init(&tasks.empty, 0, MaxQueueSize);
    sem_init(&tasks.full, 0, 0);
    tasks.front = 0;
    tasks.tail = 0;
    // HashMap threadsMap;
    Thread threads[MaxQueueSize];

    while (1) {
        sem_wait(&tasks.full);
        pthread_mutex_lock(&tasks.mutex);
        Data *TaskToRun = tasks.queue[tasks.tail];  // Check for possiblites of data corruption.
        tasks.tail = (tasks.tail + 1) % MaxQueueSize;
        printf("Task popped\n");
        pthread_mutex_unlock(&tasks.mutex);
        sem_post(&tasks.empty);

        pthread_t thread;
        if (TaskToRun->rwFlag == 0) {
            pthread_create(&thread, NULL, Read, (void *)TaskToRun);  // Data loose ho ga ider ..
        } else {
            pthread_create(&thread, NULL, Write, (void *)TaskToRun);  // Idher bhi ..
        }

        pthread_detach(thread);
    }
}
