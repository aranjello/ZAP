#ifndef ZAP_chunk_h
#define ZAP_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
    OP_ARRAY,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_RETURN,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ArrayArray constantArrays;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addArray(Chunk* chunk, Array *array);
void freeArray(Chunk *chunk, Array *arr);

#endif