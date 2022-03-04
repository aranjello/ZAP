#include <stdio.h>

#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"

VM vm; 

static void resetStack() {
  vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
}

void freeVM() {
}

void push(Array* value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Array* pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

void binaryOp(char op){
    Array *b = pop();
    Array *a = pop();
    if(a->count != b->count)
        push(createArray(1, 0));
    Array *c = createArray(a->count);
    addArray(vm.chunk, c);
    for (int i = 0; i < a->count; i++){
        double value = a->values[i];
        switch (op)
        {
        case '+': value += b->values[i]; break;
        case '-': value -= b->values[i]; break;
        case '*': value *= b->values[i]; break;
        case '/': value /= b->values[i]; break;
        
        default:
            break;
        }
        c->values[i] = value;
    }
    freeArray(vm.chunk,a);
    freeArray(vm.chunk,b);
    push(c);
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constantArrays.values[READ_BYTE()])
  for (;;) {
    #ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Array** slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(**slot);
      printf(" ]");
    }
    printf("\n");
        disassembleInstruction(vm.chunk,
                            (int)(vm.ip - vm.chunk->code));
    #endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_ARRAY: {
        Array* constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_ADD:      binaryOp('+'); break;
      case OP_SUBTRACT: binaryOp('-'); break;
      case OP_MULTIPLY: binaryOp('*'); break;
      case OP_DIVIDE:   binaryOp('/'); break;
      case OP_NEGATE:{
          Array* arr = pop();
          for (int i = 0; i < arr->count; i++)
          {
              arr->values[i] = -arr->values[i];
        }
        push(arr);
        break;
      }
      case OP_RETURN: {
        
        printValue(*pop());
        printf("\n");
        return INTERPRET_OK;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(const char* source) {
  Chunk chunk;
  initChunk(&chunk);

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();
  freeChunk(&chunk);
  return result;
}