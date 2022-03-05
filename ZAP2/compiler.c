#include <stdio.h>
#include <stdlib.h>

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

Array *tempArray = NULL;

Parser parser;

Chunk* compilingChunk;

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
    //printf("currtoken is %.*s\n",parser.current.length, parser.current.start);
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
  int constant = addArray(currentChunk(), *tempArray);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  tempArray = NULL;
  return (uint8_t)constant;
}

static void emitArray() {
  emitBytes(OP_ARRAY, makeConstant());
}

static void endCompiler() {
  emitReturn();
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

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

static void grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
  if(tempArray == NULL){
    tempArray = createArray(false,VAL_NUMBER,0);
  }
  if(tempArray->type != VAL_NUMBER){
    errorAt(&parser.previous,"array is incorrect type for number");
    return;
  }
  double value = strtod(parser.previous.start, NULL);
  writeToArray(tempArray,&value);
}

static void character(){
  if(tempArray == NULL){
    tempArray = createArray(false,VAL_CHAR,0);
  }
  if(tempArray->type != VAL_CHAR){
    errorAtCurrent("array is incorrect type for character");
    return;
  }

  writeToArray(tempArray,parser.previous.start);
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
  if(tempArray == NULL){
    tempArray = createArray(false,VAL_NUMBER,0);
  }
  if(tempArray->type != VAL_NUMBER){
    errorAt(&parser.previous,"array is incorrect type for number");
    return;
  }
  const char *arr = parser.previous.start+1;
  char *valHolder;
  if(isDigit(arr[0])){
    double val = strtod(arr,&valHolder);
    writeToArray(tempArray,&val);
    while (valHolder[0] != ']'){
      valHolder++;
      val = strtod(valHolder, &valHolder);
      writeToArray(tempArray,&val);
      
    };
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
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
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

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  compilingChunk = chunk;
  parser.hadError = false;
  parser.panicMode = false;

  advance();
  while(parser.current.type != TOKEN_EOF)
    expression();
  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}