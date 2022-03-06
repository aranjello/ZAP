#ifndef clox_value_h
#define clox_value_h

#include "common.h"
#include "table.h"

typedef enum {
  VAL_BOOL,
  VAL_NIL, 
  VAL_NUMBER,
  VAL_CHAR,
  VAL_UNKNOWN,
  VAL_KEY,
} ValueType;

typedef struct Key{
    char* value;
    int length;
    uint32_t hash;
} Key;

typedef struct Array{
  int capacity;
  int count;
  uint32_t hash;
  ValueType type;
  bool hasSubArray;
  bool garbage;
  union{
    struct Array* array;
    char *character;
    double *number;
    Key *keys;
  } as;
} Array;

typedef struct ArrayArray{
  int capacity;
  int count;
  Array* values;
} ArrayArray;



void initEmptyArray(Array* array, ValueType t);
void initArray(Array* array, bool hasSub, ValueType t, int val,...);
void * writeToArray(Array* array,const void * val);
void trashArray(Array *array);
void initValueArray(ArrayArray* array);
void*  writeValueArray(ArrayArray* array, Array value);
void freeValueArray(ArrayArray* array);
void printValue(Array value);

#endif