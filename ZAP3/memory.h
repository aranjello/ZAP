#ifndef ZAP_memory_h
#define ZAP_memory_h

#include "common.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

/*
Defines a new capacity for an array given the current array capacity
@param capacity The current array capacity
*/
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

/*
reallocates the memory at the given pointer to size it up or down
*/
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

/*
frees the memory for the given pointer
*/
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)



void* reallocate(void* pointer, size_t oldSize, size_t newSize);
#endif