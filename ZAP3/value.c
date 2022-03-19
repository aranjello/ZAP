#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "value.h"

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
initalizes a new empty array
@param array The array to initialize
@param t The type for the new array
*/
void initArray(Array* array, arrType t) {
    array->capacity = 0;
    array->count = 0;  
    initDimArray(&array->dims);
    array->t = t;
    array->as.doubles = NULL;
}

/*
Writes a new value to an existing array
@param array The array to initialize
@param value The value to add to the array
*/
void writeArray(Array* array, void* value, bool changeDims) {
    if(changeDims)
        changeArrayDims(array,1,array->dims.count-1);
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        switch(array->t){
            case(DOUBLE_ARR):
                array->as.arrays = GROW_ARRAY(struct Array*, array->as.arrays,
                                    oldCapacity, array->capacity);
                break;
            case(KEY_ARR):
                array->as.keys = GROW_ARRAY(keyString*, array->as.keys,
                                    oldCapacity, array->capacity);
                break;
            case(ARR_ARR):
                array->as.doubles = GROW_ARRAY(double, array->as.doubles,
                                    oldCapacity, array->capacity);            
                break;
            case(UNKNOWN_ARR):
                fprintf(stderr,"ERROR array was left with unknown type\n");
                exit(1);
                break;
        }
        
    }
    switch(array->t){
        case(DOUBLE_ARR):
            array->as.doubles[array->count] = *(double*)value;
            break;
        case(KEY_ARR):
            array->as.keys[array->count] = (keyString*)value;
            break;
        case(ARR_ARR):
            array->as.arrays[array->count] = (struct Array*)value;
            break;
        case(UNKNOWN_ARR):
            fprintf(stderr,"ERROR array was left with unknown type\n");
            exit(1);
            break;
    }
    array->count++;
}

/*
frees an arrays allocated memory
@param array The array to free
*/
void freeArray(Array* array) {
    switch(array->t){
        case(DOUBLE_ARR):
            FREE_ARRAY(double, array->as.doubles, array->capacity);
            break;
        case(KEY_ARR):
            FREE_ARRAY(keyString, array->as.keys, array->capacity);
            break;
        case(ARR_ARR):
            FREE_ARRAY(Array*, array->as.arrays, array->capacity);
            break;
        case(UNKNOWN_ARR):
            fprintf(stderr,"ERROR array was left with unknown type\n");
            exit(1);
            break;
    }
  
    initArray(array,array->t);
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

void copyDimArray(Array* source, Array* dest){
    for(int i = 0; i < source->dims.count; i++){
        changeArrayDims(dest,source->dims.values[i],i);
    }
}

static int printSub(Array value, int count, int offset){
    // for (int i = 0; i < value.dims->count; i++){
    //     printf("%g", value.dims->as.doubles[i]);
    // }
        if (value.dims.count == count + 1)
        {
            for (int i = 0; i < value.dims.values[count]; i++)
            {
                
                if (i > 0)
                    printf(",");
                switch (value.t)
                {
                    case DOUBLE_ARR:
                        printf("%g", value.as.doubles[i+offset]);
                        break;
                    case KEY_ARR:
                        printf("%s", value.as.keys[i+offset]->chars);
                        break;
                    case ARR_ARR:
                        printf("arr");
                        break;
                    case UNKNOWN_ARR:
                        fprintf(stderr,"ERROR array was left with unknown type\n");
                        exit(1);
                        break;
                }
        }
        offset = value.dims.values[count];
    }else{
        for (int i = 0; i < value.dims.values[count];i++){
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
prints the values contained in an array
@param value The array to print
*/
void printValue(Array* value) {
    printf("[");
    printSub(*value,0,0);
    printf("]");  
}