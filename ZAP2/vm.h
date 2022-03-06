#ifndef clox_vm_h
#define clox_vm_h

#include "codeChunk.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
  Chunk* chunk;
  uint8_t* ip;
  Array* stack[STACK_MAX];
  Array** stackTop;
  Table globals;
  Table strings;
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

#endif