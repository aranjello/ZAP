#ifndef ZAP_value_h
#define ZAP_value_h

#include "common.h"

struct Chunk;

typedef struct po{
  void *ptr;
  int offset;
} po;

typedef enum {
  //basic data types
  VAL_INT,
  VAL_DOUBLE,
  VAL_CHAR,
  VAL_BOOL,
  //key array
  VAL_KEY,
  //recursive ArrayArray
  VAL_NULL, 
  //UNKNOWN is for array being created before a type is defined
  VAL_UNKNOWN,
} ValueType;

typedef struct Key{
    char* value;
    int length;
    int loc;
    uint32_t hash;
} Key;

typedef struct arrDimensions{
  int count;
  int capacity;
  int * values;
} arrDimensions;

typedef struct Array{
  int capacity;
  int count;
  arrDimensions dims;
  uint32_t hash;
  ValueType type;
  bool garbage;
  union{
    int           *ints;
    double        *doubles;
    char          *chars;
    bool          *bools;
    Key           *keys;
  } as;
} Array;

typedef struct ArrayArray{
  int capacity;
  int count;
  Array** values;
} ArrayArray;

Array * initEmptyArray(ValueType t);
Array * createArray(ValueType t, int val,...);
void * createNewVal(Array* array,void * val,bool changeDims);
void changeArrayDims(Array* arr,int change, int dimDepth);
void trashArray(Array *array);
void initValueArray(ArrayArray* array);
void * createValueArray(ArrayArray* array , Array * arr);
void freeArray(Array *array);
void freeValueArray(ArrayArray* array);
void printValue(Array value);
void printArrayType(Array value);
#endif