#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"

VM vm; 

static void resetStack() {
  vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
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

static Array* peek(int distance) {
  return vm.stackTop[-1 - distance];
}

void binaryOp(char op){
    Array *b = pop();
    Array *a = pop();
    if(a->count != b->count)
        push(createArray(false,VAL_NUMBER,1, 0));
    Array *c = createArray(false,a->type,0);
    addArray(vm.chunk, *c);
    for (int i = 0; i < a->count; i++){
        double value = a->as.number[i];
        switch (op)
        {
        case '+': value += b->as.number[i]; break;
        case '-': value -= b->as.number[i]; break;
        case '*': value *= b->as.number[i]; break;
        case '/': value /= b->as.number[i]; break;
        
        default:
            break;
        }
        writeToArray(c, &value);
    }
    // freeArray(vm.chunk,a);
    // freeArray(vm.chunk,b);
    push(c);
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() &(vm.chunk->constantArrays.values[READ_BYTE()])
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
        if (peek(0)->type != VAL_NUMBER) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
          Array* arr = pop();
          for (int i = 0; i < arr->count; i++)
          {
            arr->as.number[i] = -arr->as.number[i];
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