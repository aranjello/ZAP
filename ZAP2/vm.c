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
    initTable(&vm.strings);
    initTable(&vm.globals);
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
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
    Array c;
    Array *d;

    if(a->count != b->count){
      initEmptyArray(&c, VAL_NIL);
      d = addRunTimeArray(vm.chunk, c);
      push(d);
      return;
    }

    initEmptyArray(&c, a->type);
 
    d = addRunTimeArray(vm.chunk, c);
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
        writeToArray(d, &value);
        
    }
    trashArray(a);
    trashArray(b);
    push(d);
}

static void getArrayVal(){
  Array *indicies = pop();
  Array* val = pop();
  Array c;
  Array *newArr;
  initEmptyArray(&c, val->type);
 
  newArr = addRunTimeArray(vm.chunk, c);

  //printf("creating new array index is %d val ther is %g\n",ind,val->as.number[ind]);
  for (int i = 0; i < indicies->count; i++){
    switch (val->type){
      case VAL_CHAR:
        writeToArray(newArr, &(val->as.character[(int)indicies->as.number[i]]));
        break;
      case VAL_NUMBER:
        writeToArray(newArr, &(val->as.number[(int)indicies->as.number[i]]));
        break;
    }
    
  }

  //printf("arr created val is %g\n",newArr->as.number[0]);
  trashArray(indicies);
  trashArray(val);
  push(newArr);
}

//0 NULL and false all evaluate to false all other values are true
static bool isFalsey(Array array){
  return array.type = VAL_BOOL;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() &(vm.chunk->constantArrays.values[READ_BYTE()])
#define READ_KEY() &(vm.chunk->keys.as.keys[READ_BYTE()])
printf("printing table vars\n");
  for (int i = 0; i < vm.strings.capacity; i++){
    if(&vm.strings.entries[i].key != NULL){
      printf("key at %d memloc %p is %s\n", i, &vm.strings.entries[i].key->value,&vm.strings.entries[i].key->value);
    }
  }
    for (;;)
    {
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
    case OP_LOOKUP:{
      getArrayVal();
      break;
    }
    case OP_ARRAY: {
          Array *constant = READ_CONSTANT();
          push(constant);
          break;
      }
      case OP_PRE_ADD:{
        double val = 0;
        Array *a = pop();
        for (int i = 0; i < a->count; i++){
          val += a->as.number[i];
        }
        a->as.number[0] = val;
        a->count = 1;
        push(a);
        break;
      }
      case OP_POP:      trashArray(pop()); break;
      case OP_GET_GLOBAL: {
        printf("getting key\n");
        Key* name = READ_KEY();
        printf("Key is %s\n",name->value);
        Array* value;
        if (!tableGet(&vm.globals, name, value)) {
          runtimeError("Undefined variable '%s'.", name->value);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        Key* k = READ_KEY();
        bool success = tableSet(&vm.globals, k, peek(0));
        //printf("write result: %s\n", success ? "pass" : "fail");
        pop();
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
      case OP_PRINT: {
        printValue(*pop());
        printf("\n");
        break;
      }
      case OP_RETURN: {
        #ifdef DEBUG_GARBAGE_COLLECTION
        printf("runtime vars in mem: ");
          for (int i = 0; i < vm.chunk->runTimeArrays.count; i++)
          {
            printValue(vm.chunk->runTimeArrays.values[i]);
          }
          printf("\n");
          printf("constant vars in mem: ");
          for (int i = 0; i < vm.chunk->constantArrays.count; i++){
            printValue(vm.chunk->constantArrays.values[i]);
          }
          printf("\n");
        #endif
        return INTERPRET_OK;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_KEY
}

InterpretResult interpret(const char* source) {
  Chunk chunk;
  initChunk(&chunk);

  if (!compile(source, &chunk,&vm)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();
  freeChunk(&chunk);
  return result;
}