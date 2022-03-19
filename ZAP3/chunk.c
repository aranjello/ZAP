#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

/*
Initializes a new code chunk
@param chunk The chunk to be initialized
*/
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initArray(&chunk->constants,ARR_ARR);
}

/*
writes a byte to the given chunks code array. These bytes can be 
op codes or memory location indexes
@param chunk The chunk to write to
@param byte The byte to write
*/
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
            oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines,
            oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

/*
adds a new array to the chunks constant list
@param chunk The chunk to add to
@param value The new array to add
*/
int addConstant(Chunk* chunk, Array* value) {
  writeArray(&chunk->constants, value,true);
  return chunk->constants.count - 1;
}

/*
frees the memroy for a chunk that has had memoery allcoated for it
@param chunk The chunk to free
*/
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeArray(&chunk->constants);
    initChunk(chunk);
}