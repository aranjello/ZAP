#include "memUtils.h"

int arrCapSize(int currSize){
    return currSize < 8 ? 8 : currSize * 2;
}

void* resizeArr(void *arrPtr, int newSize){
    if(newSize == 0){
        free(arrPtr);
    }
    arrPtr = realloc(arrPtr, newSize);
    if(arrPtr == NULL)
        printf("error realloc failed\n");
    return arrPtr;
}