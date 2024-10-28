#include <stdio.h>
#include "queue.c"
#include <pthread.h>
#include <stdbool.h>

// Node structure for hash map entries
typedef struct Node
{
    char *usernameid;
    Queue queue;
    pthread_mutex_t *qlock;

} Node;
// Hash map structure
typedef struct HashMap
{
    Node *table[TABLE_SIZE];
} HashMap;

// Hash function to map keys to indexes
unsigned int hash(int key)
{
    return key % TABLE_SIZE;
}

// Function to create a new node
Node *createNode()
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    initQueue(&newNode->queue); // Initialize queue for the node
    newNode->qlock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(newNode->qlock, NULL);
    return newNode;
}

// Initialize the hash map
void initHashMap(HashMap *map)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        map->table[i] = NULL;
    }
}

void insert(HashMap *map, data value, char *userid)
{

    int freeIndex = NULL;
    bool foundFree = false;
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        Node *current = map->table[i];

        if (current != NULL && current->usernameid == userid)
        {
            enqueue(&current->queue, value);
            return;
        }
        else if (current == NULL && foundFree == false)
        {
            freeIndex = i;
            foundFree = true;
        }
    }

    // If the key doesn't exist, create a new node
    Node *newNode = createNode();
    enqueue(&newNode->queue, value);
    newNode->usernameid = userid;
    map->table[freeIndex] = newNode;
}

// Display the entire hash map
void displayHashMap(HashMap *map)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        printf("Index %d: ", i);
        Node *current = map->table[i];
        print("%s", current->usernameid);
        displayQueue(&(current->queue));
    }
}

int main()
{
    HashMap map;
    initHashMap(&map);

    // insert(&map, 10, 100);
    // insert(&map, 10, 200);
    // insert(&map, 20, 300);
    // insert(&map, 20, 400);
    // insert(&map, 30, 500);

    displayHashMap(&map);

    Queue *q = search(&map, 20);
    if (q != NULL)
    {
        printf("Queue for key 20: ");
        displayQueue(q);
    }
    else
    {
        printf("Key 20 not found\n");
    }
    freeHashMap(&map);
    return 0;
}
