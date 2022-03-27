#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

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
    if(leftSide.dims.count + rightSide.dims.count <= 2){
      if(leftSide.dims.count == 1 && leftSide.dims.values[0] == 1)
        return LEFT_ONE_TO_ALL;
      if(rightSide.dims.count == 1 && rightSide.dims.values[0] == 1)
        return RIGHT_ONE_TO_ALL;
      return OP_TYPE_ERROR;
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

static void genericAdd(void* a, void* b, void* res, ValueType t){
  #define GENERIC_ADDER(type) *(type*)res = *(type*)a + *(type*)b
  switch (t)
  {
    case VAL_INT:{
      GENERIC_ADDER(int);
      break;
    }
    case VAL_DOUBLE:{
      *(double *)res = *(double *)a + *(double *)b;
      break;
    }
  }
  #undef GENERIC_ADDER
}

static void genericSubtract(void* a, void* b, void* res, ValueType t){
  #define GENERIC_SUBTRACTER(type) *(type*)res = *(type*)a - *(type*)b
  switch (t)
  {
    case VAL_INT:{
      GENERIC_SUBTRACTER(int);
      break;
    }
    case VAL_DOUBLE:{
      GENERIC_SUBTRACTER(double);
      break;
    }
  }
  #undef GENERIC_SUBTRACTER
}

static void genericMultiply(void* a, void* b, void* res, ValueType t){
  #define GENERIC_MULTIPLIER(type) *(type*)res = *(type*)a * *(type*)b
  switch (t)
  {
    case VAL_INT:{
      GENERIC_MULTIPLIER(int);
      break;
    }
    case VAL_DOUBLE:{
      GENERIC_MULTIPLIER(double);
      break;
    }
  }
  #undef GENERIC_MULTIPLIER
}

static void genericDivide(void* a, void* b, void* res, ValueType t){
  #define GENERIC_DIVIDER(type) *(type*)res = *(type*)a / *(type*)b
  switch (t)
  {
    case VAL_INT:{
      GENERIC_DIVIDER(int);
      break;
    }
    case VAL_DOUBLE:{
      GENERIC_DIVIDER(double);
      break;
    }
  }
  #undef GENERIC_DIVIDER
}

static void* genericEqual(void* a, void* b, void* res, ValueType t){
  #define GENERIC_EQUALIZER(type) *(int*)res = *(type*)a == *(type*)b
  switch (t)
  {
    case VAL_INT:{
      GENERIC_EQUALIZER(int);
      break;
    }
    case VAL_DOUBLE:{
      GENERIC_EQUALIZER(double);
      break;
    }
    case VAL_CHAR:{
      GENERIC_EQUALIZER(char);
      break;
    }
  }
  #undef GENERIC_EQUALIZER
}

static void genericGreater(void* a, void* b, void* res, ValueType t){
#define GENERIC_GREATER(type) *(int*)res = *(type*)a > *(type*)b
  switch (t)
  {
    case VAL_INT:{
      GENERIC_GREATER(int);
      break;
    }
    case VAL_DOUBLE:{
      GENERIC_GREATER(double);
      break;
    }
  }
  #undef GENERIC_GREATER
}

static void genericLess(void* a, void* b, void* res, ValueType t){
  
#define GENERIC_LESSER(type) *(int*)res = *(type*)a < *(type*)b
  switch (t)
  {
    case VAL_INT:{
      GENERIC_LESSER(int);
      break;
    }
    case VAL_DOUBLE:{
      GENERIC_LESSER(double);
      break;
    }
  }
  #undef GENERIC_LESSER
}

static Array* oneToOneBinaryOp(Array* a, Array *b, char compOp){
  Array *res;
  switch (compOp)
  {
    case '=':
    case '<':
    case '>':
      res = addConstantArray(initEmptyArray(VAL_INT)).ptr;
      break;
    default:
      res = addConstantArray(initEmptyArray(a->type)).ptr;
  }
   
  copyArrayDims(res, a);
  for (int i = 0; i < a->count; i++){
    void *aVal;
    void *bVal;
    void *retVal;
    switch (res->type)
    {
    case VAL_INT:
       retVal = malloc(sizeof(int));
      break;
    case VAL_DOUBLE:
      retVal = malloc(sizeof(double));
      break;
    }
    switch(a->type){
      case VAL_INT:
        aVal = &a->as.ints[i];
        bVal = &b->as.ints[i];
        break;
      case VAL_DOUBLE:
        
        aVal = &a->as.doubles[i];
        bVal = &b->as.doubles[i];
        break;
      case VAL_CHAR:
        
        aVal = &a->as.chars[i];
        bVal = &b->as.chars[i];
        break;
    }
    switch(compOp){
      case '+':
        genericAdd(aVal, bVal, retVal, a->type);
        break;
      case '-':
        genericSubtract(aVal, bVal, retVal, a->type);
        break;
      case '*':
        genericMultiply(aVal, bVal, retVal, a->type);
        break;
      case '/':
        genericDivide(aVal, bVal, retVal, a->type);
        break;
      case '=':
        genericEqual(aVal, bVal, retVal, a->type);
        break;
      case '<':
        genericLess(aVal, bVal, retVal, a->type); 
       break;
      case '>':
        genericGreater(aVal, bVal, retVal, a->type);  
      break;
    }
    createNewVal(res,retVal, false);
    free(retVal);
  }
  return res;
}

static Array* oneToAllBinaryOp(Array* one, Array* all,bool swapped, char compOp){
  Array *res;
  switch (compOp)
  {
    case '=':
    case '<':
    case '>':
      res = addConstantArray(initEmptyArray(VAL_INT)).ptr;
      break;
    default:
      res = addConstantArray(initEmptyArray(all->type)).ptr;
  }
  copyArrayDims(res, all);
  for (int i = 0; i < all->count; i++){
    void *leftVal;
    void *rightVal;
    void *retVal;
    switch(res->type){
      case VAL_INT:
        retVal = malloc(sizeof(int));
        break;
      case VAL_DOUBLE:
        retVal = malloc(sizeof(double));
        break;
      
    }
    switch(all->type){
      case VAL_INT:
        
        if(swapped){
          rightVal = &one->as.ints[0];
          leftVal = &all->as.ints[i];
          break;
        }
        leftVal = &one->as.ints[0];
        rightVal = &all->as.ints[i];
        break;
      case VAL_DOUBLE:
        if(swapped){
          rightVal = &one->as.doubles[0];
          leftVal = &all->as.doubles[i];
          break;
        }
        leftVal = &one->as.doubles[0];
        rightVal = &all->as.doubles[i];
        break;
      case VAL_CHAR:
        if(swapped){
          rightVal = &one->as.chars[0];
          leftVal = &all->as.chars[i];
          break;
        }
        leftVal = &one->as.chars[0];
        rightVal = &all->as.chars[i];
        break;
    }
    switch(compOp){
      case '+':
        genericAdd(leftVal, rightVal, retVal, all->type);
        break;
      case '-':
        genericSubtract(leftVal, rightVal, retVal, all->type);
        break;
      case '*':
        genericMultiply(leftVal, rightVal, retVal, all->type);
        break;
      case '/':
        genericDivide(leftVal, rightVal, retVal, all->type);
        break;
      case '=':
        genericEqual(leftVal, rightVal, retVal, all->type);
        break;
      case '<':
        genericLess(leftVal, rightVal, retVal, all->type); 
       break;
      case '>':
        genericGreater(leftVal, rightVal, retVal, all->type);  
      break;
    }
    createNewVal(res,retVal, false);
    free(retVal);
  }
  
  return res;
}

static Array* binaryOp(Array* leftSide, Array* rightSide,char op){
  binaryOpType opType = determineBinOpType(*leftSide, *rightSide);
  switch(opType){
    case ONE_TO_ONE:
      return oneToOneBinaryOp(leftSide, rightSide, op);
    case LEFT_ONE_TO_ALL:
      return oneToAllBinaryOp(leftSide, rightSide,false, op);
    case RIGHT_ONE_TO_ALL:
      return oneToAllBinaryOp(rightSide, leftSide,true, op);
    case OP_TYPE_ERROR:
      return leftSide;
  }
}

static Array *convertArray(Array *a, ValueType t){
    Array *ret = addConstantArray(initEmptyArray(t)).ptr;
    copyArrayDims(ret, a);
    for (int i = 0; i < a->count; i++){
      switch (a->type)
      {
      case VAL_INT:
        createNewVal(ret, &a->as.ints[i], false);
        break;
      case VAL_DOUBLE:{
        int val = a->as.doubles[i];
        createNewVal(ret, &val, false);
        break;
      }
      default:
        break;
      }
    }
    return ret;
}

static Array* getArrayVal(Array *leftVal, Array *rightVal)
{
  Array *indicies = convertArray(rightVal,VAL_INT);
  Array* val = leftVal;
  Array *ret = addConstantArray(initEmptyArray(val->type)).ptr;
  for (int i = 0; i < indicies->count; i++){
    switch (val->type){
      case VAL_INT:
        createNewVal(ret,&val->as.ints[indicies->as.ints[i]],true);
        break;
      case VAL_DOUBLE:
        createNewVal(ret,&val->as.doubles[indicies->as.ints[i]],true);
        break;
      case VAL_CHAR:
        createNewVal(ret,&val->as.chars[indicies->as.ints[i]],true);
        break;
      case VAL_BOOL:
        createNewVal(ret,&val->as.bools[indicies->as.ints[i]],true);
        break;
      case VAL_KEY:
        createNewVal(ret,&val->as.keys[indicies->as.ints[i]],true);
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
  return ret;
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
  Array* res = addConstantArray(initEmptyArray(arr->type)).ptr;
  copyArrayDims(res, arr);
  for(int i = 0; i < arr->count; i += arr->dims.values[depth]){
    void *new;
    switch (res->type)
    {
      case VAL_INT:
        new = malloc(sizeof(int));
        *(int *)new = 0;
        break;
      case VAL_DOUBLE:
        new = malloc(sizeof(double));
        *(double *)new = 0;
        break;
    }

    for(int j = 0;j < arr->dims.values[depth]; j++){
      switch(res->type){
        case VAL_INT:
          *(int*)new += arr->as.ints[i+j];
          break;
        case VAL_DOUBLE:
          *(double*)new += arr->as.doubles[i+j];
          break;
      }
      
    }
    createNewVal(res,new,false);
    
    free(new);
  }
  if(arr->dims.count != 1)
    res->dims.count--;
  else
    res->dims.values[0] = 1;
  return res;
}

static Array* getIndices(Array* a){
  Array *ret = addConstantArray(initEmptyArray(VAL_INT)).ptr;
  for (int i = 0; i < a->count; i++){
    switch(a->type){
      case VAL_INT:
        if(a->as.ints[i] != 0)
          createNewVal(ret, &i, true);
        break;
      case VAL_DOUBLE:
        if(a->as.doubles[i] != 0)
          createNewVal(ret, &i, true);
        break;
    }
  }
  return ret;
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
      push(getArrayVal(pop(),pop()));
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
      Array *res = binaryOp(pop(), pop(),'>');
      if (res->type == VAL_NULL){
        runtimeError("array size mismatch for compare operation\n");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(res);
      break;
    }
    case OP_LESS:{
      Array *res = binaryOp(pop(), pop(),'<');
      if (res->type == VAL_NULL){
        runtimeError("array size mismatch for compare operation\n");
        return INTERPRET_RUNTIME_ERROR;
      }
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
        Array *a = sumDown(pop(),0);
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
      case OP_ADD:      push(binaryOp(pop(), pop(), '+')); break;
      case OP_SUBTRACT: push(binaryOp(pop(),pop(),'-'));   break;
      case OP_MULTIPLY: push(binaryOp(pop(),pop(),'*'));   break;
      case OP_DIVIDE:   push(binaryOp(pop(),pop(),'/'));   break;
      case OP_GET_NON_ZERO_INDICES: push(getIndices(pop()));break;
      case OP_PUSH_TO_ARR:{
        Array *rightArr = pop();
        Array *leftArr = pop();
        for (int i = 0; i < rightArr->count; i++){
          switch (leftArr->type)
          {
          case VAL_INT:
            createNewVal(leftArr, &rightArr->as.ints[i], true);
            break;
          case VAL_DOUBLE:
            createNewVal(leftArr, &rightArr->as.doubles[i], true);
            break;
          case VAL_CHAR:
            createNewVal(leftArr, &rightArr->as.chars[i], true);
            break;
          }
          
        }
        push(leftArr);
        break;
      }
        case OP_NEGATE:
        {
          if (peek(0)->type != VAL_DOUBLE)
          {
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
      case OP_RANDOM: {
        Array *numRand = pop();
        Array* randArray = addConstantArray(initEmptyArray(VAL_DOUBLE)).ptr;
        time_t t = time(0);
        srand(t);
        for (int i = 0; i < numRand->as.ints[0]; i++){
          double val = rand();
          val /= RAND_MAX;
          createNewVal(randArray, &val, true);
        }
        push(randArray);
        break;
      }
      case OP_PRINT: {
        printValue(*pop());
        break;
      }
      case OP_PRINT_LN: {
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