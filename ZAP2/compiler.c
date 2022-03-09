#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "tokenScanner.h"

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // = []
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_ARRAY,
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct tempArray{
  int count;
  int capacity;
  Array **values;
} tempArray;

tempArray ta;

Array* currArray;

Parser parser;

int currDepth = 0;

int totalDepth = 0;

Chunk* compilingChunk;

VM* currVM;


static void initTempArray(tempArray* array) {
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

static void* createInTemp(tempArray* array, Array * arr) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Array*, array->values,
                                    oldCapacity, array->capacity);
    }
    array->count++;
    array->values[array->count - 1] = arr;
    return &array->values[array->count-1];
}

/*
get the current chunk we are working in
@return The current chunk
*/
static Chunk* currentChunk() {
  return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    #ifdef DEBUG_TOKEN_CREATION
      if(parser.current.type == TOKEN_EOF)
        printf("currtoken is EOF\n");
      else
        printf("currtoken is %.*s Type is %d\n",parser.current.length, parser.current.start, parser.current.type);
    #endif
    if (parser.current.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitReturn() {
  emitByte(OP_RETURN);
}

static uint8_t makeConstant() {
  int constant = addArray(*currArray).offset;
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  initTempArray(&ta);
  //freeValueArray(&ta);
 // ta = (Array*)initEmptyArray(VAL_UNKNOWN);
  return (uint8_t)constant;
}

static void emitArray() {
  emitBytes(OP_ARRAY, makeConstant());      
}

static void endCompiler() {
  emitReturn();
}

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  
  uint32_t hash = hashString(parser.previous.start, parser.previous.length);
  po p = tableFindKey(&currVM->globalInterned, parser.previous.start, parser.previous.length, hash);
  if(p.ptr == NULL){
    Key k;
    p = addGlobKey(parser.previous.start,parser.previous.length);
    tableSet(&currVM->globalInterned, p.ptr, NULL);
  }
  return (uint8_t)p.offset;
}

static void defineVariable(uint8_t global) {
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_FORWARD_SLASH: emitByte(OP_DIVIDE); break;
    case TOKEN_EQUAL_EQUAL: emitByte(OP_COMPARE); break;

    default: return; // Unreachable.
  }
}

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    Array a = *(Array*)initEmptyArray(VAL_NIL);
    int constant = addArray(a).offset;
    if (constant > UINT8_MAX) {
      error("Too many constants in one chunk.");
      return;
    }
    a = *(Array*)initEmptyArray(VAL_NIL);
    emitBytes(OP_ARRAY, constant);
  }

  defineVariable(global);
}

static void expressionStatement() {
  expression();
  emitByte(OP_POP);
}

static void printStatement() {
  expression();
  emitByte(OP_PRINT);
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_ARRAY:
        return;

      default:
        ; // Do nothing.
    }

    advance();
  }
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode) synchronize();
}

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

static void ifStatement() {

  expression();
  consume(TOKEN_SEMI, "Expect ';' after condition.");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  int elseJump = emitJump(OP_JUMP);
  patchJump(thenJump);
  emitByte(OP_POP);
  if (match(TOKEN_BAR)) statement();
  patchJump(elseJump);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static void whileStatement() {
  int loopStart = currentChunk()->count;
  expression();
  consume(TOKEN_SEMI, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  do{
    statement();
  } while (!check(TOKEN_SEMI));
  advance();
  emitLoop(loopStart);
  patchJump(exitJump);
  emitByte(OP_POP);
}

static void forStatement() {
  //consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMI)) {
    // No initializer.
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }
  consume(TOKEN_SEMI, "Expect ';'.");

  int loopStart = currentChunk()->count;
  int exitJump = -1;
  if (!match(TOKEN_SEMI)) {
    expression();
    consume(TOKEN_SEMI, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  if (!match(TOKEN_SEMI)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_SEMI, "Expect ')' after for clauses.");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }
  statement();
  emitLoop(loopStart);
  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }
}

static void statement() {
  if (match(TOKEN_BANG)) {
    printStatement();
  } else if (match(TOKEN_QUESTION)) {
    ifStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  }else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else {
    expressionStatement();
  }
}

static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
  if(currArray->count == 0){
    currArray->type = VAL_NUMBER;
  }

  if(currArray->type != VAL_NUMBER){
    errorAt(&parser.previous,"array is incorrect type for number");
    return;
  }
  double val = strtod(parser.previous.start, NULL);
  createNewVal(currArray,&val);
}

static void character(bool canAssign){
  if(ta.count == 0){
    currArray = (Array*)initEmptyArray(VAL_CHAR);
  }
  if(currArray->type == VAL_UNKNOWN){
    currArray->type = VAL_CHAR;
  }
  if(currArray->type != VAL_CHAR){
    errorAtCurrent("array is incorrect type for character");
    return;
  }
  createNewVal(currArray , (char *)parser.previous.start);
}

// static void addToArray(){
//   if(tempArray == NULL){
//     errorAtCurrent("array not initialized");
//     return;
//   }
//   double value = strtod(parser.previous.start, NULL); 
//   writeToArray(tempArray,value);
  
// }





/*
Check if the input char is a letter
@param c The char to check
@return c is a letter
*/
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
}

/*
Check if the input char is a digit
@param c The char to check
@return c is a digit
*/
static bool isDigit(char c) {
  return c == '-' || c >= '0' && c <= '9';
}



static void parseArray(bool canAssign){
  const char *arr = parser.previous.start+1;
  char *valHolder;
  if(isDigit(arr[0])){
    if(ta.count == 1){
      freeArray(ta.values[0]);
      ta.values[0] = (Array*)initEmptyArray(VAL_NUMBER);
    }else{
      printf("attempt ta.count = %d cuurdepth = %d\n",ta.count,currDepth);
      freeArray(ta.values[ta.count-1]);
      ta.values[ta.count - currDepth-1]->count--;
      ta.values[ta.count-1] = createNewVal(ta.values[ta.count - currDepth - 1], initEmptyArray(VAL_NUMBER));
      //printf("t atype is %d\n", (*(Array*)createNewVal(ta.values[ta.count - currDepth - 1], initEmptyArray(VAL_NUMBER))).type);
    }
    printf("done ta val is %s\n",ta.values[ta.count-1] == NULL?"bad":"good");
    double val = strtod(arr, &valHolder);
    printf("done ta type is %s and val is %g\n",ta.values[ta.count-1]->type==VAL_NUMBER?"good":"bad",val);
    createNewVal(ta.values[ta.count - 1], &val);
    printf("done\n");
    while (valHolder[0] != ']')
    {
      valHolder++;
      val = strtod(valHolder, &valHolder);
      createNewVal(ta.values[ta.count - 1], &val);
    };
    
  }else if(isAlpha(arr[0])){
    if(ta.count == 1){
      freeArray(ta.values[0]);
      ta.values[0] = (Array*)initEmptyArray(VAL_CHAR);
    }else{
      freeArray(ta.values[ta.count-1]);
      ta.values[ta.count-1] = createNewVal(ta.values[ta.count - currDepth - 2], initEmptyArray(VAL_CHAR));
    }
    while (*arr != ']')
    {
      createNewVal(ta.values[ta.count - 1], (char *)arr);
      arr++;
    }
    createNewVal(ta.values[ta.count - 1], '\0');
    ta.values[ta.count-1]->hash = hashString(ta.values[ta.count-1]->as.character,ta.values[ta.count-1]->count);
  }
  if(ta.count < 2)
    emitArray();
  //advance();
}

static void lookup(bool canAssign){
  printf("attempting lookup\n");
  parseArray(canAssign);
  emitByte(OP_LOOKUP);
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_PLUS:
      emitByte(OP_PRE_ADD);
      break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    case TOKEN_POUND: emitByte(OP_GET_DIMS); break;
    case TOKEN_AMP: emitByte(OP_ALL); break;
    default: return; // Unreachable.
  }
}

static void namedVariable(Token name, bool canAssign) {

  uint32_t hash = hashString(name.start, name.length);
  po p = tableFindKey(&currVM->globalInterned, name.start, name.length, hash);
  if(p.ptr == NULL){
    Key k;
    p = addGlobKey(parser.previous.start,parser.previous.length);
    tableSet(&currVM->globalInterned, p.ptr, NULL);
  }
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_GLOBAL, p.offset);
  } else {
    emitBytes(OP_GET_GLOBAL, p.offset);
  }
}

static void reassignVar(){

}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

static void chain(bool canAssign);

static void createMultiDim(bool canAssign){
  
  if(ta.count > 0 ){
    ta.values[ta.count - 1]->hasSubArray = true;
    createInTemp(&ta,createNewVal(ta.values[ta.count - 1],  initEmptyArray(VAL_UNKNOWN)));
    currArray = ta.values[ta.count - 1];
  }else{
    createInTemp(&ta, initEmptyArray(VAL_UNKNOWN));
    currArray = ta.values[ta.count - 1];
  }
  parsePrecedence(PREC_ASSIGNMENT);
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

static void closeArray(bool canAssign){
  if(ta.count >= 2)
     currArray = ta.values[ta.count - 2];
  ta.count--;
  if(ta.count == 0){
    setTypes(currArray);
    emitArray();
  }
}

static bool contains(Array* parent, Array * child){
  if(!parent->hasSubArray)
    return false;
  for (int i = 0; i < parent->count; i++){
    if(&parent->as.array[i] == child)
      return true;
  }
  return false;
}

static void chain(bool canAssign){
  parsePrecedence(PREC_ASSIGNMENT);
}

static void chainChar(bool canAssign){
  character(canAssign);
  chain(canAssign);
}

static void getDims(bool canAssign){
  parsePrecedence(PREC_UNARY);
  emitByte(OP_GET_DIMS);
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ARRAY] = {parseArray, lookup, PREC_ARRAY},
    [TOKEN_LEFT_SQUARE] = {createMultiDim, NULL, PREC_NONE},
    [TOKEN_RIGHT_SQUARE] = {NULL, closeArray, PREC_ASSIGNMENT},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, chain, PREC_ASSIGNMENT},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {unary, binary, PREC_TERM},
    [TOKEN_SEMI] = {NULL, NULL, PREC_NONE},
    [TOKEN_FORWARD_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_AMP] = {unary, NULL, PREC_FACTOR},
    //[TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_ASSIGNMENT},
    [TOKEN_GREATER] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_POUND] = {unary, NULL, PREC_NONE},
    //[TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_CHAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_AND]  = {NULL,     and_,   PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    //[TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE] = {NULL, NULL, PREC_NONE},
    //[TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    //[TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    //   [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
      [TOKEN_OR]            = {NULL,     or_,   PREC_NONE},
    //   [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    //   [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    //   [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    //   [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE] = {NULL, NULL, PREC_NONE},
    //   [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    //   [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

bool compile(const char* source, Chunk* chunk, VM* vm) {
  currVM = vm;
  initScanner(source);
  compilingChunk = chunk;
  parser.hadError = false;
  parser.panicMode = false;
  initTempArray(&ta);;
  advance();
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}
