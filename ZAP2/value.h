#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct{
    int capacity;
    int count;
    double *values;
} Array;

typedef struct {
  int capacity;
  int count;
  Array** values;
} ArrayArray;

Array* createArray(int val,...);
void writeToArray(Array* array, double val);
void freeArrayVals(ArrayArray* chunk, Array *array);
void initValueArray(ArrayArray* array);
void writeValueArray(ArrayArray* array, Array* value);
void freeValueArray(ArrayArray* array);
void printValue(Array value);

#endif