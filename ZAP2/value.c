#include <stdio.h>
#include <stdarg.h>

#include "memory.h"
#include "value.h"


Array* initEmptyArray(ValueType t){
    Array* array = malloc(sizeof(Array));
    array->capacity = 0;
    array->count = 0;
    array->hash = 0;
    array->type = t;
    array->hasSubArray = false;
    array->garbage = false;
    //setting any as value to NULL clears the array values as all as values point to the same initial memeory position
    array->as.number = NULL;
    return array;
}

/*
creates and intializes a new array
@param hasSub True if the array contains sub arrays
@param t The type that the new array will be
@param val The number of values to be added to the array
@param ... Pointers to the values to be put in the array
*/
Array *createArray(bool hasSub, ValueType t,int val,...){
    va_list ptr;
    va_start(ptr, val);
    Array* array = malloc(sizeof(Array));
    *array = *(Array*)initEmptyArray(t);
    array->hasSubArray = hasSub;
    for (int i = 0; i < val; i++){
        va_arg(ptr, void *);
        switch(t){
            case VAL_NUMBER:
                createNewVal(array,ptr);
                break;
            case VAL_CHAR:
                createNewVal(array,ptr);
                break;
        }
    }
    return array;
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
        //free(array);
}

/*
Writes a new value to an already existing array
@param array The array to be added to
@param val The value to be added
*/
void * createNewVal(Array* array, void * val){
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
        case VAL_UNKNOWN:
            array->as.array = GROW_ARRAY(Array, array->as.array, oldCapacity, array->capacity);
            break;
        default:
            printf("array type not set\n");
            break;
        }
    }
    
    array->count++;
    switch (array->type)
    {
        case VAL_KEY :
            array->as.keys[array->count - 1] = *(Key *)val;
            array->as.keys[array->count - 1].loc = array->count - 1;
            return &array->as.keys[array->count - 1];
            break;
        case VAL_NUMBER : 
            array->as.number[array->count - 1] = *(double *)val;
            return &array->as.number[array->count - 1];
            break;
        case VAL_CHAR :
            array->as.character[array->count - 1] = *(char *)val;
            return &array->as.character[array->count - 1];
            break;
        case VAL_UNKNOWN :
            array->as.array[array->count - 1] = *(Array*)val;
            return &array->as.array[array->count - 1];
            break;
        default:
            printf("array type not set for return\n");
            
    }
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
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

/*
Adds a new array to the ArrayArray for a chunk\
@param array The array to add to
@param value The array to add
*/
void* createValueArray(ArrayArray* array, Array * arr) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Array, array->values,
                                    oldCapacity, array->capacity);
    }
    array->count++;
    array->values[array->count - 1] = *arr;
    return &array->values[array->count-1];
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
        array->values[i] = *(Array*)initEmptyArray(VAL_NIL);
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
    while (value.hasSubArray)
    {
        
        for (int i = 0; i < value.count; i++){
            if(i>0)
                printf(",");
            printValue(value.as.array[i]);
            
        }
        printf("]"); 
        return;
    }
    if(value.type == VAL_NIL)
        printf("VAL NIL");
    if(value.type == VAL_CHAR)
    {
        printf("%s]", value.as.character);
        return;
    }
    
    
    for (int i = 0; i < value.count; i++){
        if(i>0)
            if(value.type == VAL_NUMBER)
                printf(",");
            switch (value.type)
            {
            case VAL_UNKNOWN: 
                printf("not set");
                break;
            case VAL_NUMBER: 
                printf("%g",value.as.number[i]);
                break;
            case VAL_KEY:{
                    Key k = value.as.keys[i];
                    for (int j = 0; j < k.length; j++){
                        if (k.value[j] == '\0')
                            break;
                        printf("%c",k.value[j]);
                    }
                    break;
                }
            case VAL_FUNC:{
                printf("<fn %s>", value.as.funcs->name);
            }
            default:
                printf("dont know");
                break;
            }
        
    }
    printf("]");
}

Array* newFunction(){
    ObjFunction f;
    f.arity = 0;
    f.name = NULL;
    f.nameLength = 0;
    initChunk(&f.chunk);
    return createArray(false, VAL_FUNC, 1, f);
}