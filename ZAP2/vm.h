#ifndef ZAP_vm_h
#define ZAP_vm_h

#include "codeChunk.h"
#include "value.h"
#include "table.h"
#include "memory.h"

#define STACK_MAX 256

typedef struct localSpace{
  Table localInterned;
  Array localKeys;
  Table localVars;
} localSpace;

typedef struct VM{
  Chunk* chunk;
  uint8_t* ip;
  Array* stack[STACK_MAX];
  Array** stackTop;
  // Table globalInterned;
  // Array globKeys;
  // Table globVars;
  // int localsCount;
  // int localsCapacity;
  // localSpace *localVars;
  // ArrayArray activeArrays;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Array* value);
Array* pop();

// po createLocalSpace();
// bool internLocalString(localSpace *local, const char * value, int length);
// po addLocalKey(localSpace *local, const char * value, int length);
// bool writeLocalVar(localSpace *local, Key* k, Array* a);



#endif