#include <stdlib.h>

#include "memory.h"

/*
Reallocates a pointer to either size up or down an array
@param pointer The pointer to the begining of some allocated memory block
@param oldSize The current size of the allocated array
@param newSize The requested size of for the array
@return A pointer to the newly allocated block of memory
*/
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    UNUSED(oldSize);
    if (newSize == 0) {
    free(pointer);
    return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}