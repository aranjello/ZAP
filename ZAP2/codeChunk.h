#ifndef ZAP_chunk_h
#define ZAP_chunk_h

#include "common.h"
#include "table.h"

//A code chunk is a self contained set of instuctions as well as the lines associated with the creation of those instructions.
//Code chunks hold all of their own data interned holds the interned strings, Keys holds the keys for the vars table, data holds all data for the chunk(constant
//and dynamic)

//A list of all available bytecode operations
typedef enum
{
    //Array controls
    OP_ARRAY,
    OP_POP,
    //Moandic math ops
    OP_PRE_ADD,
    OP_NEGATE,
    //dyadic math ops
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    //loop ops
    OP_LOOP,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    //monadic function ops
    OP_PRINT,
    OP_GET_DIMS,
    //dyadic function ops
    OP_LOOKUP,
    //monadic comparison ops
    OP_NOT,
    //dyadic comparison ops
    OP_COMPARE,
    OP_ALL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    //var ops
    OP_DEFINE_GLOBAL,
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    //return op
    OP_RETURN,
} OpCode;

//A struct that defines a single code chunk
typedef struct Chunk{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    Table interned;
    Array Keys;
    Table vars;
    ArrayArray data;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);

#endif