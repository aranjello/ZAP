#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm; 
/*
resets the stackTop pointer to the beggining of the value stack
*/
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

/*
Initializes the vm
*/
void initVM() {
    resetStack();
    initTable(&vm.globals);
    initTable(&vm.strings);
}

/*
frees any memory allocated fro the vm
*/
void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
}

/*
pushes a new value onto the stack
*/
void push(Array* value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

/*
pops the most recent value from the stack
*/
Array* pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static Array* peek(int distance) {
  return vm.stackTop[-1 - distance];
}

Array* binaryOp(Array* a, Array* b, char op){
    Array* res = malloc(sizeof(Array));
    initArray(res,a->t);
    copyDimArray(a,res);
    for(int i = 0; i < a->count; i++){

        double newVal;
        switch(op){
            case('+'):
                newVal = a->as.doubles[i] + b->as.doubles[i];
                break;
            case('-'):
                newVal = a->as.doubles[i] - b->as.doubles[i];
                break;
            case('*'):
                newVal = a->as.doubles[i] * b->as.doubles[i];
                break;
        } 
        writeArray(res,&newVal,false);
    }
    addConstant(vm.chunk,res);
    return res;
}

/*
loops throuhg the instrcutions in the current chunk and performs operations
*/
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (Array*)(vm.chunk->constants.as.arrays[READ_BYTE()])
#define READ_KEY() ((Array*)(vm.chunk->constants.as.arrays[READ_BYTE()]))->as.keys
  for (;;) {
    #ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Array** slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");

        disassembleInstruction(vm.chunk,
                            (int)(vm.ip - vm.chunk->code));
    #endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
        case OP_CONSTANT: {
            Array* constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OP_NEGATE:{
            Array* input = pop();
            Array outPut;
            initArray(&outPut,input->t);
            for(int i = 0; i < input->count; i++){
                double newVal = -input->as.doubles[i];
                writeArray(&outPut,&newVal,true);
            }
            push(&outPut);
            break;
        }
        case OP_ADD:{
            push(binaryOp(pop(),pop(),'+'));
            break;
        }
        case OP_SUBTRACT:{
            push(binaryOp(pop(),pop(),'-'));
            break;
        }
        case OP_MULTIPLY:{
            push(binaryOp(pop(),pop(),'*'));
            break;
        }
        case OP_PRINT: {
            printValue(pop());
            printf("\n");
            break;
        }
        case OP_POP: pop(); break;
        case OP_DEFINE_GLOBAL: {
            Array* keyArr = READ_CONSTANT();
            keyString name = *keyArr->as.keys[0];
            tableSet(&vm.globals, &name, peek(0));
            pop();
            break;
        }
        case OP_GET_GLOBAL: {
            Array* keyArr = READ_CONSTANT();
            keyString name = *keyArr->as.keys[0];
            Array* value  = tableGet(&vm.globals, &name);
            if (value == NULL) {
            runtimeError("Undefined variable '%s'.", name.chars);
            return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_SET_GLOBAL: {
            Array* keyArr = READ_CONSTANT();
            keyString name = *keyArr->as.keys[0];
            if (tableSet(&vm.globals, &name, peek(0))) {
                tableDelete(&vm.globals, &name); 
                runtimeError("Undefined variable '%s'.", name.chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_RETURN: {
            return INTERPRET_OK;
        }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
}

/*
takes in a chunks and runs the operations in it
@param chunk The chunk to interpret
*/
InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(&vm, source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}