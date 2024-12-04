#include <sys/mman.h>
#include <string.h>

#include "BlockPointerArray.h"
#include "stdio.h"
//+++++++++++ Global Variables ++++++++++++++

#define ArenaSize 5 * 1024  // 5 KB

// DynamicBlock* Arena_start;
DynamicArray* Blocks_Register;
//+++++++++++++++++++++++++++++++++++++++++++

int allocateArena(DynamicArray* BlockPtArray, size_t MemorySize) {
    void* arena = mmap(NULL,                         // Address hint (NULL lets the kernel decide)
                       MemorySize,                   // Size of memory to map
                       PROT_READ | PROT_WRITE,       // Memory protection (read and write allowed)
                       MAP_PRIVATE | MAP_ANONYMOUS,  // Flags (no file, private mapping)
                       -1,                           // File descriptor (-1 for anonymous mapping)
                       0);                           // Offset (irrelevant for anonymous mapping)

    if (arena == MAP_FAILED) {
        perror("mmap failed to allocate memory");
        return -1;
    }

    // now mapping the whole arena as a block
    DynamicBlock* newarena = (DynamicBlock*)arena;
    newarena->blocksize = MemorySize;
    newarena->is_available = 1;
    newarena->nextBlock = NULL;
    newarena->prevBlock = NULL;
    // Arena_start = newarena; // assigning to global arena pointer
    push(BlockPtArray, newarena);
    return 0;  // successfully assigned
}

// NOTE: remembser Arena will be made when our main program will be initialized its not related to any client process

// Custom malloc for my arena
int C_malloc(int memoryRequired) {
    for (int i = 0; i < Blocks_Register->size; i++) {
        DynamicBlock* FreeBlock = Blocks_Register->arr[i];

        if (FreeBlock->is_available == 1) {  // TODO: if the block is being perfectly fit then no need to create new
            if (FreeBlock->blocksize > memoryRequired + sizeof(DynamicBlock)) {
                // Create a new block for the remaining memory
                DynamicBlock* newBlock = (DynamicBlock*)((char*)FreeBlock + sizeof(DynamicBlock) + memoryRequired);
                newBlock->is_available = 1;
                newBlock->nextBlock = FreeBlock->nextBlock;
                newBlock->prevBlock = FreeBlock;
                // FreeBlock->nextBlock->prevBlock = newBlock;
                newBlock->blocksize = FreeBlock->blocksize - (memoryRequired + sizeof(DynamicBlock));

                // Changing FreeBlock Credentials
                FreeBlock->nextBlock = newBlock;
                FreeBlock->blocksize = memoryRequired + sizeof(DynamicBlock);
                FreeBlock->is_available = 0;

                push(Blocks_Register, newBlock);
            } else {
                FreeBlock->is_available = 0;
            }

            // return (void*)((char*)FreeBlock + sizeof(DynamicBlock));
            return i;  // returning index of that block in blockRegister
        }
    }
    return -1;  // no free block found
}

int C_Free(int index) {
    DynamicBlock* BlockToFree = Blocks_Register->arr[index];

    // now checking the previous and next block if they are free
    if (BlockToFree->nextBlock->is_available == 1 && BlockToFree->prevBlock->is_available == 1) {
        // Both next and Prev are free

        BlockToFree->prevBlock->blocksize += BlockToFree->blocksize + BlockToFree->nextBlock->blocksize;

        BlockToFree->prevBlock->nextBlock = BlockToFree->nextBlock->nextBlock;

        BlockToFree->nextBlock->nextBlock->prevBlock = BlockToFree->prevBlock;

        // it will already be free (but good practice)
        BlockToFree->prevBlock->is_available = 1;
        erase(Blocks_Register, BlockToFree->nextBlock);
        erase(Blocks_Register, BlockToFree);
        return 0;
    } else if (BlockToFree->nextBlock->is_available == 1 && BlockToFree->prevBlock->is_available == 0) {
        // only next is free
        // resetting the pointers
        BlockToFree->blocksize += BlockToFree->nextBlock->blocksize;
        BlockToFree->nextBlock->nextBlock->prevBlock = BlockToFree;

        // saving BlockToFree->nextBlock separetely for deletion from BlockRegister
        DynamicBlock* BLockToErase = BlockToFree->nextBlock;

        BlockToFree->nextBlock = BLockToErase->nextBlock;
        BlockToFree->is_available = 1;

        erase(Blocks_Register, BLockToErase);
        return 0;
    } else if (BlockToFree->nextBlock->is_available == 0 && BlockToFree->prevBlock->is_available == 1) {
        // only prev is free
        BlockToFree->prevBlock->blocksize += BlockToFree->blocksize;
        BlockToFree->prevBlock->nextBlock = BlockToFree->nextBlock;
        BlockToFree->nextBlock->prevBlock = BlockToFree->prevBlock;
        BlockToFree->prevBlock->is_available = 1;
        erase(Blocks_Register, BlockToFree);

        return 0;
    } else if (BlockToFree->nextBlock->is_available == 0 && BlockToFree->prevBlock->is_available == 0) {
        // next and prev are both not free
        BlockToFree->is_available = 1;
        return 0;
    }
    perror("None of the conditions Ran in C_Free function\n");
    return -1;
}

// int main() {
//     Blocks_Register = (DynamicArray*)malloc(sizeof(DynamicArray));

//     // initializing the global block register
//     initArray(Blocks_Register, 1 * 1024);             // array to keep the poiters is 1kb
//     allocateArena(Blocks_Register, 5 * 1024 * 1024);  // but actual arena is 5MB
//     int index = C_malloc(512);
//     int value = 12;
//     char* ptr = (char*)(Blocks_Register->arr[index]) + sizeof(DynamicBlock);
    
//     strcpy(ptr, "Bhai is OP");

//     printf("%s\n", ptr);
   
// }
