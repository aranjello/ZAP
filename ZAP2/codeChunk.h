#ifndef ZAP_chunk_h
#define ZAP_chunk_h

#include "common.h"

//A code chunk is a self contained set of instuctions as well as the lines associated with the creation of those instructions.
//Code chunks do not hold their own data and instead save offsets into a global data array in the VM

//A list of all available bytecode operations
typedef enum
{
    OP_ARRAY,
    OP_LOOKUP,
    OP_PRINT,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_GET_DIMS,
    OP_COMPARE,
    OP_ALL,
    OP_LOOP,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_PRE_ADD,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_RETURN,
} OpCode;

//A struct that defines a single code chunk
typedef struct Chunk{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);

#endif