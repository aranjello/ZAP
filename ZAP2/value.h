#ifndef ZAP_value_h
#define ZAP_value_h

#include "common.h"
#include "codeChunk.h"

typedef struct Key{
    char* value;
    int length;
    int loc;
    uint32_t hash;
} Key;

typedef struct ObjFunction{
  int arity;
  Chunk chunk;
  int nameLength;
  char* name;
} ObjFunction;

typedef struct po{
  void *ptr;
  int offset;
} po;

typedef enum {
  VAL_BOOL,
  VAL_NUMBER,
  VAL_CHAR,
  VAL_FUNC,
  //NIL is for empty array
  VAL_NIL, 
  //UNKNOWN is for array being created before a type is defined
  VAL_UNKNOWN,
  VAL_EVAL,
  VAL_KEY,
  VAL_CHUNK,
  VAL_ARRAY,
} ValueType;

typedef struct Array{
  int capacity;
  int count;
  uint32_t hash;
  ValueType type;
  bool hasSubArray;
  bool garbage;
  union{
    struct Array* arrays;
    char *characters;
    double *numbers;
    Key *keys;
    ObjFunction *funcs;
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

Array *newFunction();

#endif