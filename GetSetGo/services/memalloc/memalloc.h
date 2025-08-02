
#ifndef MEMALLOC_H_
#define MEMALLOC_H_

#include "gsg_base.h"

#error "This module is under development and not ready for use."

#define HEAP_SIZE KB_to_B(64) // Define heap size

static uint8_t heap_pool[HEAP_SIZE];
static uint8_t *heap_ptr = heap_pool;

void *malloc(size_t size);
void free(void *ptr);
void mem_init(void);

#endif // MEMALLOC_H_
