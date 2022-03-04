#ifndef zap_value_h
#define zap_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
  VAL_BOOL,
  VAL_NIL, 
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;

typedef struct{
  ValueType type;
  int count;
  int capacity;
  union
  {
    struct Array *array;
    bool *boolean;
    double *number;
    Obj **obj;
  } as;
} Array;

#define IS_BOOL_ARR(value)    ((value).type == VAL_BOOL)
#define IS_NULL_ARR(value)     ((value).type == VAL_NIL)
#define IS_NUMBER_ARR(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ_ARR(value)     ((value).type == VAL_OBJ)

#define AS_OBJ_ARR(value)     ((value).as.obj)
#define AS_BOOL_ARR(value)    ((value).as.boolean)
#define AS_NUMBER_ARR(value)  ((value).as.number)

#define BOOL_VAL_ARR(ptr)   ((Value){VAL_BOOL, {.boolean = ptr}})
#define NULL_VAL_ARR           ((Value){VAL_NIL, {.number = NULL}})
#define NUMBER_VAL_ARR(ptr) ((Value){VAL_NUMBER, {.number = ptr}})
#define OBJ_VAL_ARR(object_ptr)   ((Value){VAL_OBJ, {.obj = &object_ptr}})

typedef struct {
  int capacity;
  int count;
  Array *array;
} ValueArray;

bool valuesEqual(Array a, Array b);
void initArray(Array* array);
void* getCurrArrPointer(Array *array);
void writeArray(Array* array, void* val);
void freeArray(Array* array);
void printValue(Array array, int index);
void printArray(Array array);

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Array value);
void freeValueArray(ValueArray* array);

#endif