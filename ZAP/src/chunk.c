#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

/*
initialize a new chunck to be empty
@param chunck Chunk to initialize
*/
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants); 
}

/*
free all memory being used by a chunk
@param chunck Chunk to be freed
*/
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

/*
write bytecode to a chunk
@param chunck Chunck to write to
@param byte Code to write
@param line line number of code from program
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
Add a constant value to the chunck
@param chunk Chunk to write to
@param value Value to write
*/
int addConstant(Chunk* chunk, Array value) {
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}