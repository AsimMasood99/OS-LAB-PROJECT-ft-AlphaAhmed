#include <stdio.h>
#include <stdlib.h>


// Forward declaration of the struct
typedef struct DynamicBlock DynamicBlock;

// Full struct definition
struct DynamicBlock {
    size_t blocksize;
    int is_available;
    int invalid;
    DynamicBlock* nextBlock; // Use the forward-declared type
    DynamicBlock* prevBlock;
};

// BlockPointerArray 
typedef struct {
    DynamicBlock** arr;
    int capacity;
    int size;
} DynamicArray;

void initArray(DynamicArray *array, int initialCapacity) {
    //array->arr = (DynamicBlock **)malloc(initialCapacity * sizeof(DynamicBlock));
    array->arr = (DynamicBlock **)malloc(initialCapacity * sizeof(DynamicBlock*));
    array->capacity = initialCapacity;
    array->size = 0;
}

void push(DynamicArray *array, DynamicBlock* value) {
    if (array->size == array->capacity) {
        // Double the capacity
        array->capacity *= 2;
        array->arr = (DynamicBlock **)realloc(array->arr, array->capacity * sizeof(DynamicBlock*));
    }

    array->arr[array->size++] = value;
}

// erases a value from the Dynamic Block Array (used in C_free function)
int erase(DynamicArray *array, DynamicBlock *BlkToRemove) {
    int index = -1;

    // Find the index of the value in the array
    for (int i = 0; i < array->size; i++) {
        if (array->arr[i] == BlkToRemove) {
            index = i;
            break;
        }
    }

    // If the value is not found, exit the function
    if (index == -1) {
        printf("Error: Value not found in the array.\n");
        return -1; // didnt found the required Block
    }

    // Shift all elements after the found index to the left
    for (int i = index; i < array->size - 1; i++) {
        array->arr[i] = array->arr[i + 1];
    }

    // Decrease the size of the array
    array->size--;

    // Nullify the last pointer for safety (not necesary, but good practice)
    array->arr[array->size] = NULL;
    
    // everyting went smoothly
    return 0; 
}


// void printArray(DynamicArray *array) {
//     for (int i = 0; i < array->size; i++) {
//         printf("%d ", array->arr[i]);
//     }
//     printf("\n");
// }



// int main() {
//     DynamicArray array;
//     initArray(&array, 5);
//     push(&array, 10);
//     push(&array, 20);
//     push(&array, 30);
//     push(&array, 40);
//     push(&array, 50);
//     push(&array, 60); // This will trigger capacity doubling
//     printArray(&array);
//     return 0;
// }