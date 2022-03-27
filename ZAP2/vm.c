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
    vm.chunk = NULL;
    initTable(&vm.globalInterned);
    initTable(&vm.globVars);
    vm.globKeys = *(Array*)initEmptyArray(VAL_KEY);
    initValueArray(&vm.constantArrays);
}

void freeVM() {
  freeTable(&vm.globVars);
  freeTable(&vm.globalInterned);
  freeValueArray(&vm.constantArrays);
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
  p.ptr = createNewVal(keyArray,&key,true);
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

po addConstantArray(Array* array){
  po p;
  p.ptr = createValueArray(&vm.constantArrays,array);
  p.offset = vm.constantArrays.count - 1;
  return p;
}

static Array* peek(int distance) {
  return vm.stackTop[-1 - distance];
}

typedef enum
{
  ONE_TO_ONE, //same size arrays
  ONE_TO_SOME, //smaller array is single dim and matches to lowest level dim in larger
  LEFT_ONE_TO_ALL, //smaller array is dim 1 and larger is anything more
  RIGHT_ONE_TO_ALL, //smaller array is dim 1 and larger is anything more
  OP_TYPE_ERROR,
} binaryOpType;

static binaryOpType determineBinOpType(Array leftSide, Array rightSide){
  if(leftSide.dims.count != rightSide.dims.count){
    if(leftSide.dims.count == 1 && leftSide.dims.values[0] == 1){
      if(rightSide.dims.count != 0){
        return LEFT_ONE_TO_ALL;
      }else{
        return OP_TYPE_ERROR;
      }
    }else if(rightSide.dims.count == 1 && rightSide.dims.values[0] == 1){
      if(leftSide.dims.count != 0){
        return RIGHT_ONE_TO_ALL;
      }else{
        return OP_TYPE_ERROR;
      }
    }else{
      return OP_TYPE_ERROR;
    }
  }else{
    if(leftSide.dims.count != 0 && rightSide.dims.count != 0){
      if(leftSide.dims.count == 1 && leftSide.dims.values[0] == 1)
        return LEFT_ONE_TO_ALL;
      if(rightSide.dims.count == 1 && rightSide.dims.values[0] == 1)
        return RIGHT_ONE_TO_ALL;
    }
    for (int i = 0; i < leftSide.dims.count; i++){
      if(leftSide.dims.values[i] != rightSide.dims.values[i])
        return OP_TYPE_ERROR;
    }
    return ONE_TO_ONE;
  }
  return OP_TYPE_ERROR;
}

static bool allTrue(Array * a){
  if(a->type == VAL_NULL)
    return false;
  //addConstantArray(initEmptyArray(VAL_DOUBLE));
  for (int i = 0; i < a->count;i++){
    switch (a->type)
    {
    case VAL_INT:
      if(a->as.ints[i] == 0)
        return false;
      break;
    case VAL_DOUBLE:
      if(a->as.doubles[i] == 0)
        return false;
      break;
    case VAL_CHAR:
      if(a->as.chars[i] == '\0')
        return false;
      break;
    }
    
  }
  return true;
}

static bool anyTrue(Array * a){
  if(a->type == VAL_NULL)
    return false;
  //addConstantArray(initEmptyArray(VAL_DOUBLE));
  for (int i = 0; i < a->count;i++){
    switch (a->type)
    {
    case VAL_INT:
      if(a->as.ints[i] != 0)
        return true;
      break;
    case VAL_DOUBLE:
      if(a->as.doubles[i] != 0)
        return true;
      break;
    case VAL_CHAR:
      if(a->as.chars[i] != '\0')
        return true;
      break;
    }
    
  }
  return false;
}

static Array* getArraySize(Array * a){
  Array *res = addConstantArray(initEmptyArray(VAL_INT)).ptr;
  for (int i = 0; i < a->dims.count; i++){
    createNewVal(res, &a->dims.values[i], true);
  }
  return res;
}

static void* genericAdd(void* a, void* b, void* res, ValueType t){
  switch (t)
  {
  case VAL_INT:{
    *(int*)res = *(int *)a + *(int *)b;
    break;
  }
  default:
    break;
  }
}

static void* genericSubtract(void* a, void* b, ValueType t){
  switch (t)
  {
  case VAL_INT:{
    int val = *(int *)a - *(int *)b;
    return &val;
  }
  default:
    break;
  }
}

static void* genericEqual(void* a, void* b, ValueType t){
  switch (t)
  {
  case VAL_INT:{
    int val = *(int *)a == *(int *)b;
    return &val;
  }
  default:
    break;
  }
}

static void* genericGreater(void* a, void* b, ValueType t){
  switch (t)
  {
  case VAL_INT:{
    int val = *(int *)a > *(int *)b;
    return &val;
  }
  default:
    break;
  }
}

static void* genericLess(void* a, void* b, ValueType t){
  switch (t)
  {
  case VAL_INT:{
    int val = *(int *)a < *(int *)b;
    return &val;
  }
  default:
    break;
  }
}

static Array* oneToOneBinaryOp(Array* a, Array *b, char compOp){
  Array *res = addConstantArray(initEmptyArray(a->type)).ptr;
  copyArrayDims(res, a);
  for (int i = 0; i < a->count; i++){
    void *aVal;
    void *bVal;
    void *retVal;
    switch(res->type){
      case VAL_INT:
        retVal = malloc(sizeof(int));
        aVal = &a->as.ints[i];
        bVal = &b->as.ints[i];
        break;
      case VAL_DOUBLE:
        aVal = &a->as.doubles[i];
        bVal = &b->as.doubles[i];
        break;
    }
    switch(compOp){
      case '+':
        genericAdd(aVal, bVal, retVal, res->type);
        createNewVal(res,retVal, false);
        break;
      case '-':
        createNewVal(res,genericSubtract(aVal,bVal,res->type), false);
        break;
      case '=':
        createNewVal(res,genericEqual(aVal,bVal,res->type), false);
        break;
      case '<':
        createNewVal(res,genericGreater(aVal,bVal,res->type), false);
        break;
      case '>':
        createNewVal(res,genericLess(aVal,bVal,res->type), false);
        break;
    }
  }
  return res;
}

static Array* oneToAllBinaryOp(Array* one, Array* all, char compOp){
  Array *res = addConstantArray(initEmptyArray(one->type)).ptr;
  copyArrayDims(res, all);
  for (int i = 0; i < all->count; i++){
    void *leftVal;
    void *rightVal;
    void *retVal;
    switch(res->type){
      case VAL_INT:
        leftVal = &one->as.ints[0];
        rightVal = &all->as.ints[i];
        break;
      case VAL_DOUBLE:
        leftVal = &one->as.doubles[0];
        rightVal = &all->as.doubles[i];
        break;
    }
    switch(compOp){
      case '+':
        genericAdd(leftVal, rightVal, retVal, res->type);
        createNewVal(res,retVal, false);
        break;
      case '-':
        createNewVal(res,genericSubtract(leftVal,rightVal,res->type), false);
        break;
      case '=':
        createNewVal(res,genericEqual(leftVal,rightVal,res->type), false);
        break;
      case '<':
        createNewVal(res,genericGreater(leftVal,rightVal,res->type), false);
        break;
      case '>':
        createNewVal(res,genericLess(leftVal,rightVal,res->type), false);
        break;
    }
  }
  return res;
}

static Array* binaryOp(Array* leftSide, Array* rightSide,char op){
  binaryOpType opType = determineBinOpType(*leftSide, *rightSide);
  switch(opType){
    case ONE_TO_ONE:
      return oneToOneBinaryOp(leftSide, rightSide, op);
    case LEFT_ONE_TO_ALL:
      return oneToAllBinaryOp(leftSide, rightSide, op);
    case RIGHT_ONE_TO_ALL:
      return oneToAllBinaryOp(rightSide, leftSide, op);
    case OP_TYPE_ERROR:
      return leftSide;
  }
}


static void getArrayVal(){
  Array *indicies = pop();
  Array* val = pop();
  Array *c = (Array*)initEmptyArray(VAL_INT);
 
  po p = addConstantArray(c);

  //printf("creating new array index is %d val ther is %g\n",ind,val->as.doubles[ind]);
  for (int i = 0; i < indicies->count; i++){
    switch (val->type){
      case VAL_INT:
        createNewVal(p.ptr,&val->as.ints[indicies->as.ints[i]],true);
        break;
      case VAL_DOUBLE:
        createNewVal(p.ptr,&val->as.doubles[indicies->as.ints[i]],true);
        break;
      case VAL_CHAR:
        createNewVal(p.ptr,&val->as.chars[indicies->as.ints[i]],true);
        break;
      case VAL_BOOL:
        createNewVal(p.ptr,&val->as.bools[indicies->as.ints[i]],true);
        break;
      case VAL_KEY:
        createNewVal(p.ptr,&val->as.keys[indicies->as.ints[i]],true);
        break;
      case VAL_NULL:
      case VAL_UNKNOWN:
        //should be impossible
        exit(1);
    }
    
  }

  //printf("arr created val is %g\n",newArr->as.doubles[0]);
  trashArray(indicies);
  trashArray(val);
  push(p.ptr);
}

//0 NULL and false all evaluate to false all other values are true
static bool isTruthy(Array array){
  switch (array.type)
  {
  case VAL_INT:
    return array.as.ints[0] != 0;
    break;
  case VAL_DOUBLE:
    return array.as.doubles[0] != 0;
    break;
  case VAL_CHAR:
    return array.as.chars[0] != '\0';
    break;
  case VAL_BOOL:
    return array.as.bools[0];
    break;
  }
  return false;
}

static Array* sumDown(Array* arr, int depth){
  depth = arr->dims.count-1;
  Array* res = addConstantArray(initEmptyArray(VAL_DOUBLE)).ptr;
  for (int i = 0; i < arr->dims.count; i++){
    changeArrayDims(res,arr->dims.values[i],i);
  }
  for(int i = 0; i < arr->count; i += arr->dims.values[depth]){
    double new = 0;
    
    for(int j = 0;j < arr->dims.values[depth]; j++){
      new += arr->as.doubles[i+j];
    }
    createNewVal(res,&new,false);
  }
  if(arr->dims.count != 1)
    res->dims.count--;
  else
    res->dims.values[0] = 1;
  return res;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.constantArrays.values[READ_BYTE()])
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
    case OP_GET_TYPE:
      printArrayType(*pop());
      printf("\n");
      break;
    case OP_ALL:{
      Array *res = initEmptyArray(VAL_DOUBLE);
      double num = 0;
      if(allTrue(pop())){
        num = 1;
      }
      createNewVal(res, &num,true);
      push(addConstantArray(res).ptr);
      break;
    }
    case OP_ANY:{
      Array *res = initEmptyArray(VAL_DOUBLE);
      double num = 0;
      if(anyTrue(pop())){
        num = 1;
      }
      createNewVal(res, &num,true);
      push(addConstantArray(res).ptr);
      break;
    }
    case OP_COMPARE:{
      Array *res = binaryOp(pop(), pop(),'=');
      if (res->type == VAL_NULL){
        runtimeError("array size mismatch for compare operation\n");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(res);
      break;
    }
    case OP_GREATER:{
      Array* res = addConstantArray(initEmptyArray(VAL_DOUBLE)).ptr;
      
      double val = 1;
      if(pop()->as.doubles[0] > pop()->as.doubles[0]){
        val = 0;
      }
      createNewVal(res,&val,true);
      push(res);
      break;
    }
    case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        Array peekArray = *peek(0);
        if (peekArray.dims.count > 1 || (peekArray.dims.count == 1 && peekArray.dims.values[0] > 1))
        {
          runtimeError("array must be one value to evalute truth\n");
          return INTERPRET_RUNTIME_ERROR;
        }
        if (!isTruthy(peekArray)) vm.ip += offset;
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
      case OP_DOT_PROD:{
        Array* multRes = binaryOp(pop(),pop(),'*');
        Array* dotRes = sumDown(multRes,1);
        push(dotRes);
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
        tableSet(&vm.globVars, k, peek(0));
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
        push(binaryOp(pop(), pop(), '+'));
        break;
      case OP_SUBTRACT: push(binaryOp(pop(),pop(),'-')); break;
      case OP_MULTIPLY: push(binaryOp(pop(),pop(),'*')); break;
      case OP_DIVIDE:   push(binaryOp(pop(),pop(),'/')); break;
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
        // Array *temp = addConstantArray(*initEmptyArray(vm.stack[slot]->type)).ptr;
        // for (int i = 0; i < vm.stack[slot]->count; i++){
        //   createNewVal(temp, &vm.stack[slot]->as.doubles[i]);
        // }
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
  
  if (!compile(&vm,source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();
  return result;
}