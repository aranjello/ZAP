#ifndef ZAP_memory_h
#define ZAP_memory_h

#include "common.h"

/*
determines what the new size of an array should be given its current size
@param capacity The current capacity of an array
*/
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

/*
Reallocates a pointer and returns the new pointer location
@param type The type of the array being reallocated
@param pointer The pointer to the current location of the array
@param oldCount the current number of elements in the array
@param newCount the requested new size of the array
*/
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

/*
Frees the data at a given pointer
@param the current array type
@param pointer the pointer to the data
@param oldcount the current count of the data
*/
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif