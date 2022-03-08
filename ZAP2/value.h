#ifndef ZAP_value_h
#define ZAP_value_h

#include "common.h"

typedef struct Key{
    char* value;
    int length;
    int loc;
    uint32_t hash;
} Key;

typedef struct po{
  void *ptr;
  int offset;
} po;

typedef enum {
  VAL_BOOL,
  VAL_NUMBER,
  VAL_CHAR,
  //NIL is for empty array
  VAL_NIL, 
  //UNKNOWN is for array being created before a type is defined
  VAL_UNKNOWN,
  VAL_EVAL,
  VAL_KEY,
} ValueType;

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

Array * initEmptyArray(ValueType t);
Array * createArray(bool hasSub, ValueType t, int val,...);
void * createNewVal(Array* array,void * val);
void trashArray(Array *array);
void initValueArray(ArrayArray* array);
void * createValueArray(ArrayArray* array , Array * arr);
void freeArray(Array *array);
void freeValueArray(ArrayArray* array);
void printValue(Array value);

#endif