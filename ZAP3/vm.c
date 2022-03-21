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
    vm.frameCount = 0;
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    function* function = frame->function;
    size_t instruction = frame->ip - ((Chunk*)function->chunk)->code - 1;
    fprintf(stderr, "[line %d] in ", 
            ((Chunk*)function->chunk)->lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }
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

static bool call(function* function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.",
        function->arity, argCount);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = ((Chunk*)function->chunk)->code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Array* callee, int argCount) {
    switch (callee->t) {
        case FUNC_ARR: 
            return call(callee->as.funcs[0], argCount);
        default:
            break; // Non-callable object type.
    }
    runtimeError("Can only call functions and classes.");
    return false;
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
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    addConstant((Chunk*)frame->function->chunk,res);
    return res;
}

bool isFalsey(Array* a){
    if(a->dims.count == 0)
        return false;
    switch(a->t){
        case(DOUBLE_ARR):
            return a->as.doubles[0] == 0;
        case(KEY_ARR):
            return a->as.keys[0]->length == 0;
        case(ARR_ARR):
            return true;
            break;
        case(UNKNOWN_ARR):
            fprintf(stderr,"ERROR array was left with unknown type\n");
            exit(1);
            break;
    }
    return false;
}

/*
loops throuhg the instrcutions in the current chunk and performs operations
*/
static InterpretResult run() {
CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (Array*)(((Chunk*)frame->function->chunk)->constants.as.arrays[READ_BYTE()])
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

        disassembleInstruction(&frame->function->chunk,
        (int)(frame->ip - frame->function->chunk.code));
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
        case OP_LOOP: {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }
        case OP_JUMP: {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0))) frame->ip += offset;
            break;
        }
        case OP_CALL: {
            int argCount = READ_BYTE();
            if (!callValue(peek(argCount), argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frameCount - 1];
            break;
        }
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
        case OP_GET_LOCAL: {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]); 
            break;
        }
        case OP_SET_LOCAL: {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
         }
        case OP_RETURN: {
            Array* result = pop();
            vm.frameCount--;
            if (vm.frameCount == 0) {
                pop();
                return INTERPRET_OK;
            }

            vm.stackTop = frame->slots;
            push(result);
            frame = &vm.frames[vm.frameCount - 1];
            break;
        }
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
}

/*
takes in a chunks and runs the operations in it
@param chunk The chunk to interpret
*/
InterpretResult interpret(const char* source) {
    // Chunk chunk;
    // initChunk(&chunk);

    // if (!compile(&vm, source)) {
    //     freeChunk(&chunk);
    //     return INTERPRET_COMPILE_ERROR;
    // }

    // vm.chunk = &chunk;
    // vm.ip = vm.chunk->code;
    function* function = compile(&vm,source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;
    Array* func = malloc(sizeof(Array));
    initArray(func,FUNC_ARR);
    writeArray(func,function,true);
    push(func);
    call(function, 0);
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = ((Chunk*)function->chunk)->code;
    frame->slots = vm.stack;

    return run();
}