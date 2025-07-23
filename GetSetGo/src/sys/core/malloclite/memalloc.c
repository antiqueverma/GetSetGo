
#include "memalloc.h"

typedef struct malloc_blocks_t {
    size_t size; // Size of the block
    uint8_t *ptr; // Pointer to the next block
} malloc_blocks_t;

malloc_blocks_t mallocBlocks[HEAP_SIZE / 2] = {0}; // Array to hold malloc blocks

static uint8_t heap_pool[HEAP_SIZE];
static uint16_t maxAvlBlockSize = HEAP_SIZE;


void *malloc(size_t size) 
{
    if ((size == 0) || (size > HEAP_SIZE)) 
    {
        return NULL; // Invalid size or not enough space
    }

    void *ptr = heap_ptr; // Allocate memory from the current pointer
    heap_ptr += size; // Move the pointer forward by the allocated size
    maxAvlBlockSize -= size;
    return ptr; // Return the allocated memory address
}

void free(void *ptr) 
{
    if (ptr == NULL) 
    {
        return; // Nothing to free
    }

    // Reset the heap pointer to the start of the heap pool
    heap_ptr = heap_pool;
    maxAvlBlockSize = HEAP_SIZE; // Reset available block size
}

static void *find_free_block(size_t size) 
{
    if (size > maxAvlBlockSize) 
    {
        return NULL; // Not enough space available
    }
    return heap_ptr; // Return the current pointer as the free block
}