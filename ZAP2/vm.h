#ifndef ZAP_vm_h
#define ZAP_vm_h

#include "codeChunk.h"
#include "value.h"
#include "table.h"
#include "memory.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  Function* function;
  uint8_t* ip;
  Array* slots;
} CallFrame;

typedef struct localSpace{
  Table localInterned;
  Array localKeys;
  Table localVars;
} localSpace;

typedef struct VM{
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  Array* stack[STACK_MAX];
  Array** stackTop;
  Table globalInterned;
  // Array globKeys;
  Table globVars;
  //ArrayArray constantArrays;
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

bool internGlobString(const char * value, int length);
po addGlobKey(const char * value, int length);
bool writeGlobVar(Key* k, Array* a);
po addConstantArray(Array *array);



#endif