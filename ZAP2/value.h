#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
  VAL_BOOL,
  VAL_NIL, 
  VAL_NUMBER,
  VAL_CHAR,
  VAL_UNKNOWN,
} ValueType;

typedef struct{
  int capacity;
  int count;
  ValueType type;
  bool hasSubArray;
  union{
    struct Array* array;
    char *character;
    double *number;
  } as;
} Array;

typedef struct {
  int capacity;
  int count;
  Array* values;
} ArrayArray;

Array* createArray(bool hasSub, ValueType t, int val,...);
void writeToArray(Array* array,const void * val);
void freeArrayVals(ArrayArray* chunk, Array *array);
void initValueArray(ArrayArray* array);
void writeValueArray(ArrayArray* array, Array value);
void freeValueArray(ArrayArray* array);
void printValue(Array value);

#endif