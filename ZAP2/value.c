#include <stdio.h>
#include <stdarg.h>

#include "memory.h"
#include "value.h"
#include "codeChunk.h"

/*
Initialize a arrDimensions array
@param dims A pointer to the arrDimensions array to initialize
*/
static void initDimArray(arrDimensions* dims){
    dims->count = 0;
    dims->capacity = 0;
    dims->values = NULL;
}

/*
Creates a new empty array with the given type and returns a pointer to it
@param t The value type for the new array
*/
Array* initEmptyArray(ValueType t){
    Array* array = malloc(sizeof(Array));
    array->capacity = 0;
    array->count = 0;
    array->hash = 0;
    array->type = t;
    array->garbage = false;
    //setting any as value to NULL clears the array values as all as values point to the same initial memeory position due to the union
    array->as.doubles = NULL;
    initDimArray(&array->dims);
    return array;
}

/*
creates and intializes a new array
@param hasSub True if the array contains sub arrays
@param t The type that the new array will be
@param val The number of values to be added to the array
@param ... Pointers to the values to be put in the array
*/
Array *createArray(ValueType t,int numVals,...){
    va_list ptr;
    va_start(ptr, numVals);
    Array* array = malloc(sizeof(Array));
    *array = *(Array*)initEmptyArray(t);
    //array->hasSubArray = hasSub;
    for (int i = 0; i < numVals; i++){
        va_arg(ptr, void *);
        switch(t){
            case VAL_DOUBLE:
                createNewVal(array,ptr,true);
                break;
            case VAL_CHAR:
                createNewVal(array,ptr,true);
                break;
            default:
                printf("not yet implemented\n");
                exit(1);
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
            case VAL_DOUBLE:
                FREE_ARRAY(double, array->as.doubles, array->capacity);
                break;
            case VAL_CHAR:
                FREE_ARRAY(char, array->as.chars, array->capacity);
                break;
            default:
                printf("not yet implemented\n");
                exit(1);
                break;

        }
        //free(array);
}

void changeArrayDims(Array* arr,int change, int dimDepth){
    //for unitialized dims
    if(dimDepth < 0)
        dimDepth = 0;
    if(dimDepth+1 > arr->dims.capacity){
        int oldCapacity = arr->dims.capacity;
        arr->dims.capacity = GROW_CAPACITY(dimDepth);
        arr->dims.values = GROW_ARRAY(int,arr->dims.values,oldCapacity,arr->dims.capacity);
    }
    while(arr->dims.count < dimDepth+1){
        arr->dims.values[arr->dims.count] = 0;
        arr->dims.count++;
    }
    
    arr->dims.values[dimDepth]+= change;
}

/*
Writes a new value to an already existing array
@param array The array to be added to
@param val The value to be added
*/
void * createNewVal(Array* array, void * val, bool changeDims){
    if(changeDims)
        changeArrayDims(array,1,array->dims.count-1);
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        switch (array->type)
        {
        case VAL_KEY:
            array->as.keys = GROW_ARRAY(Key, array->as.keys,
                                    oldCapacity, array->capacity);
            break;
        case VAL_DOUBLE:
            array->as.doubles = GROW_ARRAY(double, array->as.doubles,
                                    oldCapacity, array->capacity);
            break;
        case VAL_CHAR:
            array->as.chars = GROW_ARRAY(char,array->as.chars,
                                    oldCapacity, array->capacity);
            break;
        // case VAL_UNKNOWN:
        //     array->as.arrays = GROW_ARRAY(Array*, array->as.arrays, oldCapacity, array->capacity);
        //     break;
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
        case VAL_DOUBLE : 
            array->as.doubles[array->count - 1] = *(double *)val;
            return &array->as.doubles[array->count - 1];
            break;
        case VAL_CHAR :
            array->as.chars[array->count - 1] = *(char *)val;
            return &array->as.chars[array->count - 1];
            break;
        // case VAL_UNKNOWN :
        //     array->as.arrays[array->count - 1] = (Array*)val;
        //     return array->as.arrays[array->count - 1];
        //     break;
        default:
            printf("array type not set for return\n");
            return NULL;
            break;
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
        array->values = GROW_ARRAY(Array*, array->values,
                                    oldCapacity, array->capacity);
    }
    array->count++;
    array->values[array->count - 1] = arr;
    return array->values[array->count-1];
}

/*
frees the memory used by a chunks array array
@param array The array to free
*/
void freeValueArray(ArrayArray* array) {
    for (int i = 0; i < array->count; i++){
        switch(array->values[i]->type)
        {
            case VAL_KEY:
                FREE_ARRAY(Key, array->values[i]->as.keys, array->values[i]->capacity);
                break;
            case VAL_DOUBLE:
                FREE_ARRAY(double, array->values[i]->as.doubles, array->values[i]->capacity);
                break;
            case VAL_CHAR:
                FREE_ARRAY(char, array->values[i]->as.chars, array->values[i]->capacity);
                break;  
            default:
                printf("not yet implemented\n");
                exit(1);
                break;
        }
        array->values[i] = initEmptyArray(VAL_NULL);
    }
    FREE_ARRAY(Array, array->values, array->capacity);
    initValueArray(array);
}

static int printSub(Array value, double count, int offset){
    // for (int i = 0; i < value.dims->count; i++){
    //     printf("%g", value.dims->as.doubles[i]);
    // }
        if (value.dims.count == count + 1)
        {
            for (int i = 0; i < value.dims.values[(int)count]; i++)
            {
                
                if (i > 0)
                    if (value.type == VAL_DOUBLE)
                        printf(",");
                switch (value.type)
                {
                case VAL_UNKNOWN:
                    printf("not set");
                    break;
                case VAL_DOUBLE:
                    printf("%g", value.as.doubles[i+offset]);
                    break;
                case VAL_CHAR:
                    printf("%c", value.as.chars[i+offset]);
                    break;
                case VAL_KEY:
                {
                    Key k = value.as.keys[i];
                    for (int j = 0; j < k.length; j++){
                        if (k.value[j] == '\0')
                            break;
                        printf("%c",k.value[j]);
                    }
                    break;
                }
                
            case VAL_FUNC:{
                if (value.as.funcs->name == NULL) {
                    printf("<script>");
                    return;
                }
                printf("<fn %s>", value.as.funcs->name);
                break;
            }
            default:
                printf("dont know");
                break;
            }
            
        }
        offset = value.dims.values[(int)count];
    }else{
        for (int i = 0; i < value.dims.values[(int)count];i++){
            if(i > 0)
                printf(",");
            printf("[");
            offset += printSub(value, count + 1,offset);
            printf("]");
        }
    }
    return offset;
}

/*
prints the values out of an array
@param value The array to print from
*/
void printValue(Array value) {
    // printf("value dims: ");
    // for(int i = 0; i < value.dims->count; i++){
    //     if(i>0)
    //         printf(",");
    //     printf("%g",value.dims->as.doubles[i]);
    // }
    printf("[");
    printSub(value,0,0);
    printf("]");
    // printf("[");
    // while (value.hasSubArray)
    // {
        
    //     for (int i = 0; i < value.count; i++){
    //         if(i>0)
    //             printf(",");
    //         printValue(*value.as.arrays[i]);
            
    //     }
    //     printf("]"); 
    //     return;
    // }
    // if(value.type == VAL_NULL)
    //     printf("VAL NIL");
    // if(value.type == VAL_CHAR)
    // {
    //     printf("%s]", value.as.chars);
    //     return;
    // }

    
    // for (int i = 0; i < value.count; i++){
        
        
    // }
    // printf("]");
}

Function* newFunction(){
    Function *f = malloc(sizeof(Function));
    f->arity = 0;
    f->name = NULL;
    f->nameLength = 0;
    initChunk(&f->chunk);
    return f;
}