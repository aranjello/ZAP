#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "codeChunk.h"

/*
initializes code chunck
@param chunk The chunk to initialize
*/
void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
}

/*
frees all memory used for a chunk
@param chunk The chunk to free
*/
void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  initChunk(chunk);
}

/*
writes a bytecode instruction to the chunk
@param chunk The chunk to write to
@param byte The byte to write
@param line The line the code that generated the byte came from
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

  chunk->lines[chunk->count] = line;
  chunk->code[chunk->count] = byte;
  chunk->count++;
}

