#include <stdio.h>
#include <stdarg.h>

#include "memory.h"
#include "value.h"

Array* createArray(int val,...){
    va_list ptr;
    va_start(ptr, val);
    Array* array = (Array*)malloc(sizeof(Array));
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
    for (int i = 0; i < val; i++){
        if (array->capacity < array->count + 1)
        {
            int oldCapacity = array->capacity;
            array->capacity = GROW_CAPACITY(oldCapacity);
            array->values = GROW_ARRAY(double, array->values,
                                       oldCapacity, array->capacity);
        }
        array->values[i] = va_arg(ptr,double);
        array->count++;
    }
    return array;
}

void writeToArray(Array* array, double val){
    if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(double, array->values,
                               oldCapacity, array->capacity);
  }

  array->values[array->count] = val;
  array->count++;
}

void freeArrayVals(ArrayArray* arr,Array* array){
    FREE_ARRAY(double, array->values, array->capacity);
    arr->count--;
}

// void recursiveFree(Array* array){
//     free(array);
// }

void initValueArray(ArrayArray* array) {
    // if(array->capacity != NULL){
    //     for (int i = 0; i < array->capacity; i++){
    //         recursiveFree(array->values[i]);
    //     }
    // }
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ArrayArray* array, Array* value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Array*, array->values,
                               oldCapacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

void freeValueArray(ArrayArray* array) {
    
    for (int i = 0; i < array->count; i++){
        Array *tempArr = array->values[i];
        FREE_ARRAY(double, tempArr, tempArr->capacity);
    }
    FREE_ARRAY(Array, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Array value) {
    printf("[");
    for (int i = 0; i < value.count; i++){
        if(i>0)
            printf(",");
        printf("%g", value.values[i]);
    }
    printf("]");
}