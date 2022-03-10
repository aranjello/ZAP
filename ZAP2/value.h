#ifndef ZAP_value_h
#define ZAP_value_h

#include "common.h"

struct Chunk;

typedef struct Function{
  int arity;
  struct Chunk* chunk;
  int nameLength;
  char* name;
} Function;

typedef struct Key{
    char* value;
    int length;
    int loc;
    uint32_t hash;
} Key;

typedef struct Evaluator{
  void *evaluator;
} Evaluator;

typedef struct po{
  void *ptr;
  int offset;
} po;

typedef enum {
  //basic data types
  VAL_INT,
  VAL_DOUBLE,
  VAL_FLOAT,
  VAL_CHAR,
  VAL_BOOL,
  //function array
  VAL_FUNC,
  //key array
  VAL_KEY,
  //evaluation array
  VAL_EVAL,
  //chunk array
  VAL_CHUNK,
  //recursive ArrayArray
  VAL_ARRAY,
  //NULL is for null array
  VAL_NULL, 
  //UNKNOWN is for array being created before a type is defined
  VAL_UNKNOWN,
} ValueType;

typedef struct Array{
  int capacity;
  int count;
  struct Array* dims;
  uint32_t hash;
  ValueType type;
  // bool hasSubArray;
  bool garbage;
  union{
    int           *ints;
    double        *doubles;
    float         *floats;
    char          *chars;
    bool          *bools;
    Function      *funcs;
    Key           *keys;
    Evaluator     *evals;
    //struct Chunk  *chunks;
    //struct Array  **arrays;
  } as;
} Array;

typedef struct ArrayArray{
  int capacity;
  int count;
  Array** values;
} ArrayArray;

Array * initEmptyArray(ValueType t);
Array * createArray(ValueType t, int val,...);
void * createNewVal(Array* array,void * val);
void trashArray(Array *array);
void initValueArray(ArrayArray* array);
void * createValueArray(ArrayArray* array , Array * arr);
void freeArray(Array *array);
void freeValueArray(ArrayArray* array);
void printValue(Array value);

Array *newFunction();

#endif