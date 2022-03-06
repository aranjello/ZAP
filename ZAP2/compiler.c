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

typedef void (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Array tempArray;

Parser parser;

Chunk* compilingChunk;

VM* currVM;

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
  printf("checking %.*s\n", parser.current.length, parser.current.start);
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
  int constant = addArray(currentChunk(), tempArray).offset;
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  initEmptyArray(&tempArray, VAL_UNKNOWN);
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
  Key k;
  po p = addKey(currentChunk(), k);
  Key *key = (Key*)p.ptr;
  key->value = malloc(sizeof(char) * parser.previous.length);
  memcpy(key->value, parser.previous.start, parser.previous.length);
  key->hash = hashString(parser.previous.start, parser.previous.length);
  key->length = parser.previous.length;
  
  printf("hashed %s pointer is %p and got %lu\n", key->value, key->value, key->hash);
  
  tableSet(&currVM->strings, key, NULL);
  Array* a;
  bool found = tableGet(&currVM->strings, p.ptr, a);
  printf("the val is %s\n", found ? "found" : "not found");
  //Key *testK = tableFindkey(&currVM.strings, k->value, k->length, k->hash);
  return (uint8_t)p.offset;
}

static void defineVariable(uint8_t global) {
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void binary() {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_FORWARD_SLASH:         emitByte(OP_DIVIDE); break;
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
    Array a;
    initEmptyArray(&a, VAL_NIL);
    int constant = addArray(currentChunk(), a).offset;
    if (constant > UINT8_MAX) {
      error("Too many constants in one chunk.");
      return;
    }
    initEmptyArray(&a, VAL_NIL);
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
    printf("definining var\n");
    varDeclaration();
  } else {
    printf("checking statemetn\n");
    statement();
  }

  if (parser.panicMode) synchronize();
}

static void statement() {
  if (match(TOKEN_BANG)) {
    printStatement();
  } else {
    expressionStatement();
  }
}

static void grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
  if(&tempArray == NULL){
    initEmptyArray(&tempArray,VAL_NUMBER);
  }
  if(tempArray.type != VAL_NUMBER){
    errorAt(&parser.previous,"array is incorrect type for number");
    return;
  }
  double value = strtod(parser.previous.start, NULL);
  writeToArray(&tempArray,&value);
}

static void character(){
  if(&tempArray == NULL){
    initEmptyArray(&tempArray, VAL_CHAR);
  }
  if(tempArray.type != VAL_CHAR){
    errorAtCurrent("array is incorrect type for character");
    return;
  }

  writeToArray(&tempArray,parser.previous.start);
}

// static void addToArray(){
//   if(tempArray == NULL){
//     errorAtCurrent("array not initialized");
//     return;
//   }
//   double value = strtod(parser.previous.start, NULL); 
//   writeToArray(tempArray,value);
  
// }

static void chain(){
  parsePrecedence(PREC_ASSIGNMENT);
}

static void chainChar(){
  character();
  chain();
}

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
  return c >= '0' && c <= '9';
}



static void parseArray(){
  
  const char *arr = parser.previous.start+1;
  char *valHolder;
  if(isDigit(arr[0])){
    initEmptyArray(&tempArray, VAL_NUMBER);
    double val = strtod(arr,&valHolder);
    writeToArray(&tempArray,&val);
    while (valHolder[0] != ']'){
      valHolder++;
      val = strtod(valHolder, &valHolder);
      writeToArray(&tempArray,&val);
      
    };
  }else if(isAlpha(arr[0])){
    initEmptyArray(&tempArray, VAL_CHAR);
    while (arr[0] != ']')
    {
      writeToArray(&tempArray, arr);
      arr++;
    }
    char c = '\0';
    writeToArray(&tempArray, &c);
    tempArray.hash = hashString(tempArray.as.character,tempArray.count);
  }
  
  emitArray();
  //consume(TOKEN_RIGHT_SQUARE, "Improperly close array");
}

static void lookup(){
  parseArray();
  emitByte(OP_LOOKUP);
}

static void unary() {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_PLUS:
      emitByte(OP_PRE_ADD);
      break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: return; // Unreachable.
  }
}

static void namedVariable(Token name) {
  
  Key *k = malloc(sizeof(Key));
  
  k->value = malloc(sizeof(char) * name.length);
  memcpy(k->value, name.start, name.length);

  k->hash = hashString(name.start, name.length);
  k->length = name.length;
  printf("hashed %s and got %lu\n", k->value, k->hash);
  
  Key *testK = tableFindKey(&currVM->strings, k->value, k->length, k->hash);
  printf("test k was %s\n", testK == NULL? "not found": "found");
  uint8_t arg = 0;
  if(testK == NULL){
    tableSet(&currVM->strings, k, NULL);
    uint8_t arg = (uint8_t)addKey(currentChunk(), *k).offset;
  }else{
    for (int i = 0; i < &currVM->strings.capacity; i++){
      if(&currVM->strings.entries[i] != NULL &&
      memcmp(&currVM->strings.entries[i].key->value,
        k->value,k->length)){
          arg = i;
          break;
        }
    }
      
  }
  
  emitBytes(OP_GET_GLOBAL, arg);
}

static void variable() {
  namedVariable(parser.previous);
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ARRAY] = {parseArray, lookup, PREC_ARRAY},
    [TOKEN_LEFT_SQUARE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_SQUARE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, chain, PREC_ASSIGNMENT},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {unary, binary, PREC_TERM},
    [TOKEN_SEMI] = {NULL, NULL, PREC_NONE},
    [TOKEN_FORWARD_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    //[TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    //[TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_CHAR] = {character, chainChar, PREC_NONE},
    //[TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    //[TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE] = {NULL, NULL, PREC_NONE},
    //[TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    //[TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    //   [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
    //   [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
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

  prefixRule();
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule();
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
  advance();
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}