#include <stdio.h>
#include <stdlib.h>
#define INITIAL_CAPACITY 4
#define TABLE_SIZE 100
// number of user basically size of hash map

typedef struct
{

    char command; // v d u
    char filename[50];
    int filesize;
    int new_socket;
    cJSON *JsonCommand;

    // other things
    // client socket

} data;

// Queue-like dynamic array structure
typedef struct
{
    data *requestData;
    int front;
    int rear;
    int size;
    int capacity;
} Queue;

// Function to initialize a queue
void initQueue(Queue *q)
{
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->capacity = INITIAL_CAPACITY;
    q->requestData = (data *)malloc(sizeof(data) * q->capacity);
}

// Function to resize the queue
void resizeQueue(Queue *q)
{
    q->capacity *= 2;
    q->requestData = (data *)realloc(q->requestData, sizeof(data) * q->capacity);
}

// Function to enqueue (add element to the rear of the queue)
void enqueue(Queue *q, data value)
{
    if (q->size == q->capacity)
    {
        resizeQueue(q);
    }
    q->rear = (q->rear + 1) % q->capacity;
    q->requestData[q->rear] = value;
    q->size++;
}

// Function to dequeue (remove element from the front of the queue)
data dequeue(Queue *q)
{
    if (q->size == 0)
    {
        printf("Queue is empty, cannot dequeue\n");
    }
    data value = q->requestData[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return value;
}

// Function to check if the queue is empty
int isEmpty(Queue *q)
{
    return q->size == 0;
}

// Function to display the queue
void displayQueue(Queue *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }
    int i = q->front;
    int count = 0;
    while (count < q->size)
    {
        printf("%d ", q->requestData[i]);
        i = (i + 1) % q->capacity;
        count++;
    }
    printf("\n");
}

// Function to free the memory of the queue
void freeQueue(Queue *q)
{
    free(q->requestData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////