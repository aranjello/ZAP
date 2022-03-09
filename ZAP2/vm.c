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
    initTable(&vm.globalInterned);
    initTable(&vm.globVars);
    vm.globKeys = *(Array*)initEmptyArray(VAL_KEY);
    vm.localsCount = 0;
    vm.localsCapacity = 0;
    vm.localVars = NULL;
    initValueArray(&vm.activeArrays);
}

void freeVM() {
  freeTable(&vm.globVars);
  freeTable(&vm.globalInterned);
  freeValueArray(&vm.activeArrays);
  FREE_ARRAY(Key, vm.globKeys.as.keys,vm.globKeys.capacity);
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

static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

static po allocateNewKey(Array* keyArray,const char * value, int length){
  po p;
  Key key;
  key.value = malloc(sizeof(char) * length);
  memcpy(key.value, value, length);
  key.hash = hashString(value,length);
  key.length = length;
  p.ptr = createNewVal(keyArray,&key);
  p.offset = keyArray->count-1;
  return p;
}

bool internGlobString(const char * value, int length){
  return tableSet(&vm.globalInterned, allocateNewKey(&vm.globKeys,value, length).ptr, NULL);
}

po addGlobKey(const char * value, int length){
  return allocateNewKey(&vm.globKeys,value, length);
}

bool writeGlobalVar(Key* k, Array* a){
  return tableSet(&vm.globVars, k, a);
}

po createLocalSpace(VM *vm){
   if (vm->localsCapacity < vm->localsCount + 1) {
        int oldCapacity = vm->localsCapacity;
        vm->localsCapacity = GROW_CAPACITY(oldCapacity);
        vm->localVars = GROW_ARRAY(localSpace, vm->localVars,
                                    oldCapacity, vm->localsCapacity);
   }
   po p;
   p.ptr = &vm->localVars[vm->localsCount];
   p.offset = vm->localsCount;
   vm->localsCount++;
}

bool internLocalString(localSpace *local, const char * value, int length){

  return tableSet(&local->localInterned, allocateNewKey(&local->localKeys, value, length).ptr, NULL);
}

po addLocalKey(localSpace *local, const char * value, int length){
  return allocateNewKey(&local->localKeys, value, length);
}

bool writeLocalVar(localSpace *local, Key* k, Array* a){
  return tableSet(&local->localVars, k, a);
}

po addArray(Array array){
  po p;
  p.ptr = createValueArray(&vm.activeArrays,&array);
  p.offset = vm.activeArrays.count - 1;
  return p;
}


static Array* peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static void setTypes(Array * a){
  for (int i = 0; i < a->count; i++){
    if(a->hasSubArray)
      setTypes(&a->as.array[i]);
  }
  Array *temp = a;
  while(temp->hasSubArray){
    temp = temp->as.array;
  }
  a->type = temp->type;
}

static bool all(Array * a){
  if(a->type == VAL_NIL)
    return false;
  Array *res = addArray(*initEmptyArray(VAL_NUMBER)).ptr;
  if(a->hasSubArray){
    for (int i = 0; i < a->count; i++){
      if(!all(&a->as.array[i]))
        return false;
    }
  }else{
    for (int i = 0; i < a->count; i++){
      if(a->as.number[i] == 0)
        return false;
    }
  }
  return true;
}

static Array* getArraySize(Array * a){
  Array c = *(Array*)initEmptyArray(VAL_NUMBER);
  po p = addArray(c);
  Array *temp = a;
  while(temp->hasSubArray){
    double b = (double)temp->count;
    createNewVal(p.ptr, &b);
    temp = temp->as.array;
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
        createNewVal(res, compareArrays(&tempa->as.array[i], &tempb->as.array[i]));
      }
      res->type = VAL_NUMBER;
  }else if(tempa->hasSubArray || tempb->hasSubArray){
    res = addArray(*initEmptyArray(VAL_NIL)).ptr;
  }else{
    res = addArray(*initEmptyArray(VAL_NUMBER)).ptr;
    for (int i = 0; i < a->count; i++){
      double result = (double)(a->as.number[i] == b->as.number[i]);
      createNewVal(res,&result);
    }
  }
  //res->type = VAL_NUMBER;
  return res;
}


static bool binaryOp(char op){
    Array *b = pop();
    Array *a = pop();
    Array c = *(Array*)initEmptyArray(VAL_NUMBER);

    if(!all(compareArrays(getArraySize(a),getArraySize(b)))){
      runtimeError("array size mismatch for operation %c\n", op);
      return false;
    }
    po p = addArray(c);
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
  Array c = *(Array*)initEmptyArray(VAL_NUMBER);
 
  po p = addArray(c);

  //printf("creating new array index is %d val ther is %g\n",ind,val->as.number[ind]);
  for (int i = 0; i < indicies->count; i++){
    switch (val->type){
      case VAL_CHAR:
        createNewVal(p.ptr,&val->as.character[(int)indicies->as.number[i]]);
        break;
      case VAL_NUMBER:
        createNewVal(p.ptr,&val->as.number[(int)indicies->as.number[i]]);
        break;
    }
    
  }

  //printf("arr created val is %g\n",newArr->as.number[0]);
  trashArray(indicies);
  trashArray(val);
  push(p.ptr);
}

//0 NULL and false all evaluate to false all other values are true
static bool isFalsey(Array array){
  if(array.type == VAL_NUMBER){
    return array.as.number[0] == 0;
  }
  return false;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() &(vm.activeArrays.values[READ_BYTE()])
#define READ_SHORT() \
    (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_KEY() &(vm.globKeys.as.keys[READ_BYTE()]);
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
        disassembleInstruction(&vm,vm.chunk,
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
      Array *res = initEmptyArray(VAL_NUMBER);
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
      if (res->type == VAL_NIL){
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
          val += a->as.number[i];
        }
        a->as.number[0] = val;
        a->count = 1;
        push(a);
        break;
      }
      case OP_POP:      trashArray(pop()); break;
      case OP_GET_GLOBAL: {
        Key* name = READ_KEY();
        
        Array* value = tableGet(&vm.globVars, name);
        if (value == NULL) {
          runtimeError("Undefined variable '%s'.", name->value);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        Key* k = READ_KEY();
        bool success = tableSet(&vm.globVars, k, peek(0));
        //printf("write result: %s\n", success ? "pass" : "fail");
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        Key* key = READ_KEY();
        if (tableSet(&vm.globVars, key, peek(0))) {
          tableDelete(&vm.globVars, key); 
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