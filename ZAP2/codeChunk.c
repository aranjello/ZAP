#include <stdlib.h>

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


static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

static po allocateNewKey(Array* keyArray,const char * value, int length){
  po p;
  Key key;
  key.value = malloc(sizeof(char) * length);
  memcpy(key.value, value, length);
  key.hash = hashString(value,length);
  key.length = length;
  p.ptr = createNewVal(keyArray,&key);
  p.offset = keyArray->count-1;
  return p;
}

bool internString(Chunk* c,const char * value, int length){
  return tableSet(&c->interned, allocateNewKey(c->Keys.as.keys,value, length).ptr, NULL);
}

po addKey(Chunk* c,const char * value, int length){
  return allocateNewKey(c->Keys.as.keys,value, length);
}

bool writeVar(Chunk* c,Key* k, Array* a){
  return tableSet(&c->vars, k, a);
}

po addArray(Chunk* c,Array array){
  po p;
  p.ptr = createValueArray(&c->data,&array);
  p.offset = c->data.count - 1;
  return p;
}