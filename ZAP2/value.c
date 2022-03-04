#include <stdio.h>
#include <stdarg.h>

#include "memory.h"
#include "value.h"

/*
creates and intializes a new array
@param hasSub True if the array contains sub arrays
@param t The type that the new array will be
@param val The number of values to be added to the array
@param ... Pointers to the values to be put in the array
*/
Array* createArray(bool hasSub, ValueType t,int val,...){
    va_list ptr;
    va_start(ptr, val);
    Array* array = (Array*)malloc(sizeof(Array));
    array->as.number = NULL;
    array->capacity = 0;
    array->count = 0;
    array->type = t;
    array->hasSubArray = hasSub;
    for (int i = 0; i < val; i++){
        if (array->capacity < array->count + 1)
        {
            int oldCapacity = array->capacity;
            array->capacity = GROW_CAPACITY(oldCapacity);
            switch (t)
            {
            case VAL_NUMBER:
                array->as.number = GROW_ARRAY(double, array->as.number,
                                       oldCapacity, array->capacity);
                break;
            case VAL_CHAR:
                array->as.character = GROW_ARRAY(char,array->as.character,
                                       oldCapacity, array->capacity);
            }
            
        }
        switch (t)
        {
        case VAL_NUMBER : array->as.number[i] = *(double*)va_arg(ptr,void*);
            break;
        case VAL_CHAR : array->as.character[i] = *(char*)va_arg(ptr,void*);
            break;
        }
        array->count++;
    }
    return array;
}

/*
Writes a new value to an already existing array
@param array The array to be added to
@param val The value to be added
*/
void writeToArray(Array* array,const void* val){
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        switch (array->type)
        {
        case VAL_NUMBER:
            array->as.number = GROW_ARRAY(double, array->as.number,
                                    oldCapacity, array->capacity);
            break;
        case VAL_CHAR:
            array->as.character = GROW_ARRAY(char,array->as.character,
                                    oldCapacity, array->capacity);
        }
    }
  
    switch (array->type)
    {
    case VAL_NUMBER : array->as.number[array->count] = *(double*)val;
        break;
    case VAL_CHAR : array->as.character[array->count] = *(char*)val;
        break;
    }
    array->count++;
}

/*
Frees the memory being used by an array
@param arr The array array containing the array to be freed
@param array The array to free
*/
void freeArrayVals(ArrayArray* arr,Array* array){
    switch(array->type)
    {
        case VAL_NUMBER:
            FREE_ARRAY(double, array->as.number, array->capacity);
            break;
        case VAL_CHAR:
            FREE_ARRAY(char, array->as.character, array->capacity);
            break;
    }
    
    //arr->count--;
}

/*
Initializes the array array of a chunk
@param array The array to initialize
*/
void initValueArray(ArrayArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

/*
Adds a new array to the ArrayArray for a chunk\
@param array The array to add to
@param value The array to add
*/
void writeValueArray(ArrayArray* array, Array value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Array, array->values,
                               oldCapacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

/*
frees the memory used by a chunks array array
@param array The array to free
*/
void freeValueArray(ArrayArray* array) {
    for (int i = 0; i < array->count; i++){
        freeArrayVals(array, &array->values[i]);
    }
    FREE_ARRAY(Array, array->values, array->capacity);
    initValueArray(array);
}

/*
prints the values out of an array
@param value The array to print from
*/
void printValue(Array value) {
    printf("[");
    for (int i = 0; i < value.count; i++){
        if(i>0)
            printf(",");
            switch (value.type)
            {
            case VAL_NUMBER: printf("%g",value.as.number[i]);
                break;
            case VAL_CHAR: printf("%c",value.as.character[i]);
                break;
            }
        
    }
    printf("]");
}