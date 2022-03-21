#ifndef ZAP_chunk_h
#define ZAP_chunk_h

#include "common.h"
#include "value.h"

// The list of opcodes for instructions the vm can run
typedef enum {
    OP_CONSTANT,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_CALL,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_NIL,
    OP_PRINT,
    OP_POP,
    OP_RETURN,
} OpCode;

/*
A self contained "chunk" of code. Contains instructions for a 
code section and the lines where each in struction comes from.
*/
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    Array constants;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Array* value);
void freeChunk(Chunk* chunk);

#endif