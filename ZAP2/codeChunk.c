#include <stdlib.h>

#include "codeChunk.h"
#include "memory.h"
/*
initializes code chunck
@param chunk The chunk to initialize
*/
void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initValueArray(&chunk->constantArrays);
  initValueArray(&chunk->runTimeArrays);
  initEmptyArray(&chunk->keys, VAL_KEY);
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

/*
Add an array to the chunks list of tracked arrays
@param chunk The chunk to add to
@param value The array to add
*/
po addArray(Chunk* chunk, Array value) {
  po p;
  p.ptr = writeValueArray(&chunk->constantArrays, value);
  p.offset = chunk->constantArrays.count - 1;
  return p;
}

Array* addRunTimeArray(Chunk *chunk, Array array){
  
  writeValueArray(&chunk->runTimeArrays, array);
  
  return &chunk->runTimeArrays.values[chunk->runTimeArrays.count - 1];
}

po addKey(Chunk* chunk, Key k){
  po p;
  p.ptr = writeToArray(&chunk->keys, &k);
  p.offset = chunk->keys.count - 1;
  return p;
}

/*
frees all memory used for a chunk
@param chunk The chunk to free
*/
void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  FREE_ARRAY(Key, chunk->keys.as.keys, chunk->keys.capacity);
  freeValueArray(&chunk->constantArrays);
  freeValueArray(&chunk->runTimeArrays);
  initChunk(chunk);
}