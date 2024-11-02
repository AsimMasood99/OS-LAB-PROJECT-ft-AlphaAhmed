#include "hashmap.c"
#include <pthread.h>
#include <semaphore.h>
#include "tasksMap.h"

typedef struct
{
    char *username;
    int rwFlag;
    char *filename;
} Job;

const int MaxQueueSize = 128;

typedef struct
{
    int front;
    int tail;
    sem_t empty;
    sem_t full;
    pthread_mutex_t mutex;
    Job queue[MaxQueueSize];
} JobQueue;

JobQueue tasks;

typedef struct
{
    Job Task;
    int suspended;
    pthread_t thread;
} Thread;

void addTask(Job task)
{
    sem_wait(&tasks.empty);

    pthread_mutex_lock(&tasks.mutex);

    tasks.queue[tasks.front] = task;
    tasks.front = (tasks.front + 1) % MaxQueueSize;

    // Release mutual exclusion
    pthread_mutex_unlock(&tasks.mutex);

    // Signal that buffer now has one more item
    sem_post(&tasks.full);
}

void *ReadWrite() {}

void initThreadPoll(Thread threads[])
{
    for (int i = 0; i < MaxQueueSize; i++)
    {
        threads[i].suspended = 1;
        pthread_create(&threads[i].thread, NULL, ReadWrite, (void *)args); // Not sure what to send here rn.
    }
}

fileHandler(void *args)
{
    sem_init(&tasks.empty, 0, MaxQueueSize);
    sem_init(&tasks.full, 0, 0);
    HashMap threadsMap;
    Thread threads[MaxQueueSize];

    initThreadPoll(threads);

    while (1)
    {
        sem_wait(&tasks.full);
        pthread_mutex_lock(&tasks.mutex);
        Job TaskToRun = tasks.queue[tasks.tail];
        tasks.tail = (tasks.tail + 1) % MaxQueueSize;
        pthread_mutex_unlock(&tasks.mutex);
        sem_post(&tasks.empty);
        // ThreadsMap wala kam yahan ho ga.
    }
}

/*

The sem_init function is used to initialize a semaphore in C. Its signature is as follows:

int sem_init(sem_t *sem, int pshared, unsigned int value);
Here's a breakdown of the arguments used in your call to sem_init:

sem_t *sem:

This is a pointer to the semaphore that you want to initialize. In your example, &empty is passed, which means you're initializing the semaphore named empty. It should be defined as a sem_t type variable.
int pshared:

This argument indicates whether the semaphore is shared between processes (pshared is non-zero) or only between threads of the same process (pshared is zero). In your case, 0 is used, meaning that the semaphore will be used only by threads within the same process.
unsigned int value:

This is the initial value of the semaphore. In your example, BUFFER_SIZE is passed, which sets the initial count of the semaphore to the size of the buffer. This indicates how many empty slots are available in the buffer initially.


*/