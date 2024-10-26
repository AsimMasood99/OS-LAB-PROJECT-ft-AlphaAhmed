#include<stdio.h>
#include "queue.c"
#include<pthread.h>



// Node structure for hash map entries
typedef struct Node {
    int key;
    char user_id;
    Queue queue;
    pthread_mutex_lock queueLock;
    
} Node;
// Hash map structure
typedef struct HashMap {
    Node* table[TABLE_SIZE];
} HashMap;

// Hash function to map keys to indexes
unsigned int hash(int key) {
    return key % TABLE_SIZE;
}

// Function to create a new node
Node* createNode(int key) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->key = key;
    initQueue(&newNode->queue);  // Initialize queue for the node
    return newNode;
}

// Initialize the hash map
void initHashMap(HashMap* map) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        map->table[i] = NULL;
    }
}

// Insert a key-value pair into the hash map
void insert(HashMap* map, int key, data value) {
    unsigned int index = hash(key);
    Node* current = map->table[index];

    // If the key already exists, enqueue the value
    if (current != NULL) {
            enqueue(&current->queue, value);
            return;
    }
   
    // If the key doesn't exist, create a new node
    Node* newNode = createNode(key);
    enqueue(&newNode->queue, value);
    // Insert the new node at the head of the chain
    map->table[index] = newNode;
}


// Display the entire hash map
void displayHashMap(HashMap* map) {
    for (int i = 0; i < TABLE_SIZE; i++)
     {
        printf("Index %d: ", i);
        Node* current = map->table[i];
        print("%s",current->user_id);
        displayQueue(&(current->queue));
     }
}



int main() {
    HashMap map;
    initHashMap(&map);

    // insert(&map, 10, 100);
    // insert(&map, 10, 200);
    // insert(&map, 20, 300);
    // insert(&map, 20, 400);
    // insert(&map, 30, 500);

    displayHashMap(&map);

    Queue* q = search(&map, 20);
    if (q != NULL) {
        printf("Queue for key 20: ");
        displayQueue(q);
    } else {
        printf("Key 20 not found\n");
    }
    freeHashMap(&map);
    return 0;
}
