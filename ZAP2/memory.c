#include <stdlib.h>

#include "memory.h"

/*
Reallocates or frees the data at a given pointer given the pointer, its current size and the new size it should be
@param pointer Pointer to the data
@param oldSize The old size of the data
@param newSize The requested new size of the data
*/
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  (void)oldSize;
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }
  void* result = realloc(pointer, newSize);
  if (result == NULL) exit(1);
  return result;
}
