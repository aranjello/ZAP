#include <stdio.h>
#include <stdarg.h>

#include "memory.h"
#include "value.h"


void initEmptyArray(Array* array, ValueType t){
    array->capacity = 0;
    array->count = 0;
    array->type = t;
    array->garbage = false;
    array->hasSubArray = false;
    //setting any as value to NULL clears the array values as all as values point to the same initial memeory position
    array->as.number = NULL;
}

/*
creates and intializes a new array
@param hasSub True if the array contains sub arrays
@param t The type that the new array will be
@param val The number of values to be added to the array
@param ... Pointers to the values to be put in the array
*/
void createArray(Array* array, bool hasSub, ValueType t,int val,...){
    va_list ptr;
    va_start(ptr, val);
    array->as.number = NULL;
    array->capacity = 0;
    array->count = 0;
    array->type = t;
    array->hasSubArray = hasSub;
    array->garbage = false;
    for (int i = 0; i < val; i++){
        if (array->capacity < array->count + 1)
        {
            int oldCapacity = array->capacity;
            array->capacity = GROW_CAPACITY(oldCapacity);
            switch (t)
            {
            case VAL_KEY:
                array->as.keys = GROW_ARRAY(Key, array->as.keys, 
                                        oldCapacity, array->capacity);
                break;
            case VAL_NUMBER:
                array->as.number = GROW_ARRAY(double, array->as.number,
                                        oldCapacity, array->capacity);
                break;
            case VAL_CHAR:
                array->as.character = GROW_ARRAY(char,array->as.character,
                                        oldCapacity, array->capacity);
                break;
            }
            
        }
        switch (t)
        {
        case VAL_KEY :
            array->as.keys[i] = *(Key*)va_arg(ptr,void*);
            break;
        case VAL_NUMBER : 
            array->as.number[i] = *(double*)va_arg(ptr,void*);
            break;
        case VAL_CHAR : 
            array->as.character[i] = *(char*)va_arg(ptr,void*);
            break;
        }
        array->count++;
    }
}

void freeArray(Array* array){
    switch(array->type)
        {
            case VAL_KEY:
                FREE_ARRAY(Key, array->as.keys, array->capacity);
                break;
            case VAL_NUMBER:
                FREE_ARRAY(double, array->as.number, array->capacity);
                break;
            case VAL_CHAR:
                FREE_ARRAY(char, array->as.character, array->capacity);
                break;
        }
        free(array);
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
        case VAL_KEY:
            array->as.keys = GROW_ARRAY(Key, array->as.keys,
                                    oldCapacity, array->capacity);
            break;
        case VAL_NUMBER:
            array->as.number = GROW_ARRAY(double, array->as.number,
                                    oldCapacity, array->capacity);
            break;
        case VAL_CHAR:
            array->as.character = GROW_ARRAY(char,array->as.character,
                                    oldCapacity, array->capacity);
            break;
        }
    }
  
    switch (array->type)
    {
    case VAL_KEY : array->as.keys[array->count] = *(Key*)val;
        break;
    case VAL_NUMBER : array->as.number[array->count] = *(double*)val;
        break;
    case VAL_CHAR : array->as.character[array->count] = *(char*)val;
        break;
    }
    array->count++;
}

/*
Marks array as garbage for later collection
*/
void trashArray(Array* array){
    array->garbage = true;
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
        switch(array->values[i].type)
        {
            case VAL_KEY:
                FREE_ARRAY(Key, array->values[i].as.keys, array->values[i].capacity);
                break;
            case VAL_NUMBER:
                FREE_ARRAY(double, array->values[i].as.number, array->values[i].capacity);
                break;
            case VAL_CHAR:
                FREE_ARRAY(char, array->values[i].as.character, array->values[i].capacity);
                break;
        }
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
            if(value.type == VAL_NUMBER || value.type == VAL_CHAR && i < value.count-1)
                printf(",");
            switch (value.type)
            {
            case VAL_NIL: 
                printf("VAL NIL\n");
                break;
            case VAL_NUMBER: 
                printf("%g",value.as.number[i]);
                break;
            case VAL_CHAR: 
                if(value.as.character[i]=='\0')
                    break;
                printf("%c",value.as.character[i]);
                break;
            }
        
    }
    printf("]");
}