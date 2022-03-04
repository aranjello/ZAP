#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

void initArray(Array* array){
  array->as.number = NULL;
  array->capacity = 0;
  array->count = 0;
}

void* getCurrArrPointer(Array* array){
  switch (array->type)
  {
  case VAL_BOOL:
    return array->as.boolean;
    break;
  case VAL_NUMBER:
    return array->as.number;
    break;
  case VAL_OBJ:
    return array->as.obj;
    break;
  
  default:
    break;
  }
}

void writeArray(Array* array, void* val){
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    switch (array->type)
    {
    case VAL_BOOL:
      array->as.boolean  = GROW_ARRAY(bool, array->as.boolean,
                               oldCapacity, array->capacity);
      break;
    case VAL_NUMBER:
      array->as.number  = GROW_ARRAY(double, array->as.number,
                               oldCapacity, array->capacity);
      break;
    case VAL_OBJ:
      array->as.obj  = GROW_ARRAY(Obj*, array->as.obj,
                               oldCapacity, array->capacity);
      break;
    
    default:
      break;
    }
  }
  switch (array->type)
    {
    case VAL_BOOL:
      array->as.boolean[array->count] = *((bool*)val);
      break;
    case VAL_NUMBER:
      array->as.number[array->count] = *((double*)val);
      break;
    case VAL_OBJ:
      array->as.obj[array->count] = ((Obj*)val);
      break;
    
    default:
      break;
    }
  array->count++;
}
void freeArray(Array* array){
  switch (array->type)
    {
    case VAL_BOOL:
      FREE_ARRAY(bool, array->as.boolean, array->capacity);
      break;
    case VAL_NUMBER:
      FREE_ARRAY(double, array->as.number, array->capacity);
      break;
    case VAL_OBJ:
      FREE_ARRAY(Obj*, array->as.obj, array->capacity);
      break;
    default:
      break;
    }
  initValueArray(array);
}
void printArray(Array array){
  for (int i = 0; i < array.count; i++){
    switch (array.type)
    {
      case VAL_BOOL:
        printf("%s\n",array.as.boolean[i]);
        break;
      case VAL_NUMBER:
        printf("%f\n", array.as.number[i]);
        break;
      case VAL_OBJ:
        printObject(array.as.obj[i]);
        break;
      default:
        break;
    }
  }
}


void initValueArray(ValueArray* array) {
  array->array = NULL;
  array->capacity = 0;
  array->count = 0;
}

void writeValueArray(ValueArray* array, Array value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->array = GROW_ARRAY(Array, array->array,
                               oldCapacity, array->capacity);
  }

  array->array[array->count] = value;
  array->count++;
}

void freeValueArray(ValueArray* array) {
  FREE_ARRAY(Array, array->array, array->capacity);
  initValueArray(array);
}

void printValue(Array value, int index) {
  
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL_ARR(value)[index] ? "true" : "false");
      break;
    case VAL_NIL: printf("null"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER_ARR(value)[index]); break;
    case VAL_OBJ: printObj(AS_OBJ_ARR(value)[index]); break;
  }
  
}

bool valuesEqual(Array a, Array b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL_ARR(a) == AS_BOOL_ARR(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER_ARR(a) == AS_NUMBER_ARR(b);
    case VAL_OBJ:    return AS_OBJ_ARR(a) == AS_OBJ_ARR(b);
    default:         return false; // Unreachable.
  }
}