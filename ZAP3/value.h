#ifndef ZAP_value_h
#define ZAP_value_h

#include "common.h"

//list of the possible types of arrays
typedef enum{
    DOUBLE_ARR,
    KEY_ARR,
    FUNC_ARR,
    ARR_ARR,
    UNKNOWN_ARR,
}arrType;

typedef struct{
    int length;
    char* chars;
    uint32_t hash;
} keyString;

typedef struct {
  int arity;
  struct Chunk* chunk;
  keyString* name;
} function;

typedef struct{
  int count;
  int capacity;
  int * values;
} arrDimensions;

//the vase array struct
typedef struct {
    int capacity;
    int count;
    arrDimensions dims;
    arrType t;
    union
    {
        double* doubles;
        keyString** keys;
        function** funcs;
        struct Array** arrays;
    }as;
} Array;

void initArray(Array* array, arrType t);
void writeArray(Array* array, void* value, bool changeDims);
void freeArray(Array* array);

void changeArrayDims(Array* arr,int change, int dimDepth);
void copyDimArray(Array* source, Array* dest);

void printValue(Array* val);

function* newFunction();

#endif