#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define maxQueueSize 16

typedef struct
{
    char *username;
    char *filename;
    int fileSize; 
    int socket;
    int rwFlag; // 0 for read. 1 for write.
    int completed;
} Data;

// Queue-like dynamic array structure
typedef struct
{
    Data requestData[maxQueueSize];
    int front;
    int rear;
    int size; 
    sem_t empty;
    sem_t full;
    pthread_mutex_t mutex;
} Queue;

// Function to initialize a queue
void initQueue(Queue *q)
{
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    sem_init(&q->empty,0,maxQueueSize);
    sem_init(&q->full, 0, 0);
}

// // Function to enqueue (add element to the rear of the queue)
// void enqueue(Queue *q, Data value)
// {
//     // kia yahan par bhi bounded buffer lagana ki zarorat ha? 
//     if (q->size == maxQueueSize)
//     {
//         resizeQueue(q);
//     }
//     q->rear = (q->rear + 1) % q->capacity;
//     q->requestData[q->rear] = value;
//     q->size++;
// }

// // Function to dequeue (remove element from the front of the queue)
// Data dequeue(Queue *q)
// {
//     if (q->size == 0)
//     {
//         printf("Queue is empty, cannot dequeue\n");
//     }
//     Data value = q->requestData[q->front];
//     q->front = (q->front + 1) % q->capacity;
//     q->size--;
//     return value;
// }

// // Function to check if the queue is empty
// int isEmpty(Queue *q)
// {
//     return q->size == 0;
// }
// // Function to free the memory of the queue
// void freeQueue(Queue *q)
// {
//     free(q->requestData);
// }
