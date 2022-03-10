#include <stdio.h>
#include <stdarg.h>
#include <string.h>

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
    // initTable(&vm.globalInterned);
    // initTable(&vm.globVars);
    // vm.globKeys = *(Array*)initEmptyArray(VAL_KEY);
    // vm.localsCount = 0;
    // vm.localsCapacity = 0;
    // vm.localVars = NULL;
    // initValueArray(&vm.activeArrays);
}

void freeVM() {
  // freeTable(&vm.globVars);
  // freeTable(&vm.globalInterned);
  // freeValueArray(&vm.activeArrays);
  // FREE_ARRAY(Key, vm.globKeys.as.keys,vm.globKeys.capacity);
  //mem for any local vars shoudl be cleared already
}

void push(Array* value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Array* pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

// bool internGlobString(const char * value, int length){
//   return tableSet(&vm.globalInterned, allocateNewKey(&vm.globKeys,value, length).ptr, NULL);
// }

// po addGlobKey(const char * value, int length){
//   return allocateNewKey(&vm.globKeys,value, length);
// }

// bool writeGlobalVar(Key* k, Array* a){
//   return tableSet(&vm.globVars, k, a);
// }

// po createLocalSpace(VM *vm){
//    if (vm->localsCapacity < vm->localsCount + 1) {
//         int oldCapacity = vm->localsCapacity;
//         vm->localsCapacity = GROW_CAPACITY(oldCapacity);
//         vm->localVars = GROW_ARRAY(localSpace, vm->localVars,
//                                     oldCapacity, vm->localsCapacity);
//    }
//    po p;
//    p.ptr = &vm->localVars[vm->localsCount];
//    p.offset = vm->localsCount;
//    vm->localsCount++;
// }

// bool internLocalString(localSpace *local, const char * value, int length){

//   return tableSet(&local->localInterned, allocateNewKey(&local->localKeys, value, length).ptr, NULL);
// }

// po addLocalKey(localSpace *local, const char * value, int length){
//   return allocateNewKey(&local->localKeys, value, length);
// }

// bool writeLocalVar(localSpace *local, Key* k, Array* a){
//   return tableSet(&local->localVars, k, a);
// }

// po addArray(Array array){
//   po p;
//   p.ptr = createValueArray(&vm.activeArrays,&array);
//   p.offset = vm.activeArrays.count - 1;
//   return p;
// }


static Array* peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static void setTypes(Array * a){
  for (int i = 0; i < a->count; i++){
    if(a->hasSubArray)
      setTypes(&a->as.arrays[i]);
  }
  Array *temp = a;
  while(temp->hasSubArray){
    temp = temp->as.arrays;
  }
  a->type = temp->type;
}

static bool all(Array * a){
  if(a->type == VAL_NULL)
    return false;
  Array *res = addArray(*initEmptyArray(VAL_DOUBLE)).ptr;
  if(a->hasSubArray){
    for (int i = 0; i < a->count; i++){
      if(!all(&a->as.arrays[i]))
        return false;
    }
  }else{
    for (int i = 0; i < a->count; i++){
      if(a->as.doubles[i] == 0)
        return false;
    }
  }
  return true;
}

static Array* getArraySize(Array * a){
  Array c = *(Array*)initEmptyArray(VAL_DOUBLE);
  po p = addArray(c);
  Array *temp = a;
  while(temp->hasSubArray){
    double b = (double)temp->count;
    createNewVal(p.ptr, &b);
    temp = temp->as.arrays;
  }
  double b = (double)temp->count;
  createNewVal(p.ptr, &b);
  return p.ptr;
}

static Array* compareArrays(Array* a, Array *b){
  Array *res;
  Array *tempa = a;
  Array *tempb = b;
  if(tempa->hasSubArray && tempb->hasSubArray){
      res = addArray(*initEmptyArray(VAL_UNKNOWN)).ptr;
      res->hasSubArray = true;
      for (int i = 0; i < a->count; i++){
        createNewVal(res, compareArrays(&tempa->as.arrays[i], &tempb->as.arrays[i]));
      }
      res->type = VAL_DOUBLE;
  }else if(tempa->hasSubArray || tempb->hasSubArray){
    res = addArray(*initEmptyArray(VAL_NULL)).ptr;
  }else{
    res = addArray(*initEmptyArray(VAL_DOUBLE)).ptr;
    for (int i = 0; i < a->count; i++){
      double result = (double)(a->as.doubles[i] == b->as.doubles[i]);
      createNewVal(res,&result);
    }
  }
  //res->type = VAL_DOUBLE;
  return res;
}


static bool binaryOp(char op){
    Array *b = pop();
    Array *a = pop();
    Array c = *(Array*)initEmptyArray(VAL_DOUBLE);

    if(!all(compareArrays(getArraySize(a),getArraySize(b)))){
      runtimeError("array size mismatch for operation %c\n", op);
      return false;
    }
    po p = addArray(c);
    for (int i = 0; i < a->count; i++){
        double value = a->as.doubles[i];
        switch (op)
        {
          case '+': value += b->as.doubles[i]; break;
          case '-': value -= b->as.doubles[i]; break;
          case '*': value *= b->as.doubles[i]; break;
          case '/': value /= b->as.doubles[i]; break;
          
          default:
              break;
        }

        createNewVal(p.ptr, &value);
        
    }
    trashArray(a);
    trashArray(b);
    push(p.ptr);
    return true;
}


static void getArrayVal(){
  Array *indicies = pop();
  Array* val = pop();
  Array c = *(Array*)initEmptyArray(VAL_DOUBLE);
 
  po p = addArray(c);

  //printf("creating new array index is %d val ther is %g\n",ind,val->as.doubles[ind]);
  for (int i = 0; i < indicies->count; i++){
    switch (val->type){
      case VAL_CHAR:
        createNewVal(p.ptr,&val->as.chars[(int)indicies->as.doubles[i]]);
        break;
      case VAL_DOUBLE:
        createNewVal(p.ptr,&val->as.doubles[(int)indicies->as.doubles[i]]);
        break;
    }
    
  }

  //printf("arr created val is %g\n",newArr->as.doubles[0]);
  trashArray(indicies);
  trashArray(val);
  push(p.ptr);
}

//0 NULL and false all evaluate to false all other values are true
static bool isFalsey(Array array){
  if(array.type == VAL_DOUBLE){
    return array.as.doubles[0] == 0;
  }
  return false;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() &(vm.chunk->data.values[READ_BYTE()])
#define READ_SHORT() \
    (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_KEY() &(vm.chunk->Keys.as.keys[READ_BYTE()]);
    for (;;)
    {
    #ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    printf("[ ");
    for (Array** slot = vm.stack; slot < vm.stackTop; slot++) {
      
      printValue(**slot);
      
    }
    printf(" ]");
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
    case OP_GET_DIMS:
      push(getArraySize(pop()));
      break;
    case OP_ALL:{
      Array *res = initEmptyArray(VAL_DOUBLE);
      double num = 0;
      if(all(pop())){
        num = 1;
      }
      createNewVal(res, &num);
      push(res);
      break;
    }
    case OP_COMPARE:{
      Array *res = compareArrays(pop(), pop());
      if (res->type == VAL_NULL){
        runtimeError("array size mismatch for compare operation\n");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(res);
      break;
    }
    case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(*peek(0))) vm.ip += offset;
        break;
      }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      vm.ip += offset;
      break;
    }
    case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        vm.ip -= offset;
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
          val += a->as.doubles[i];
        }
        a->as.doubles[0] = val;
        a->count = 1;
        push(a);
        break;
      }
      case OP_POP:      trashArray(pop()); break;
      case OP_GET_GLOBAL: {
        Key* name = READ_KEY();
        
        Array* value = tableGet(&vm.chunk->vars, name);
        if (value == NULL) {
          runtimeError("Undefined variable '%s'.", name->value);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        Key* k = READ_KEY();
        bool success = tableSet(&vm.chunk->vars, k, peek(0));
        //printf("write result: %s\n", success ? "pass" : "fail");
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        Key* key = READ_KEY();
        if (tableSet(&vm.chunk->vars, key, peek(0))) {
          tableDelete(&vm.chunk->vars, key); 
          runtimeError("Undefined variable '%s'.", key->value);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_ADD:
        if(!binaryOp('+'))
          return INTERPRET_RUNTIME_ERROR;
        break;
      case OP_SUBTRACT: binaryOp('-'); break;
      case OP_MULTIPLY: binaryOp('*'); break;
      case OP_DIVIDE:   binaryOp('/'); break;
      case OP_NEGATE:{
        if (peek(0)->type != VAL_DOUBLE) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
          Array* arr = pop();
          for (int i = 0; i < arr->count; i++)
          {
            arr->as.doubles[i] = -arr->as.doubles[i];
          }
        push(arr);
        break;
      }
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(vm.stack[slot]); 
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        vm.stack[slot] = peek(0);
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
#undef READ_SHORT
#undef READ_KEY
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
  return result;
}