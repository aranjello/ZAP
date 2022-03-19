#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "memory.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

bool parseSet = false;

Array* currArray;

int currDepth = 0;

int shallowestClosedDepth = INT_MAX;

VM* currVM;

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
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

Parser parser;
Chunk* compilingChunk;

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
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
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
  return c == '-' || (c >= '0' && c <= '9');
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

static uint8_t makeConstant(Array* value) {
    printValue(value);
    printf("\n");
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}

static void emitConstant(Array* value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
    emitReturn();
    #ifdef DEBUG_PRINT_CODE
        if (!parser.hadError) {
            disassembleChunk(currentChunk(), "code");
        }
    #endif
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

static uint8_t identifierConstant(Token* name) {
    Array* temp = malloc(sizeof(Array));
    initArray(temp,KEY_ARR);
    keyString* string = tableFindKey(&currVM->strings,name->start,
                                            name->length,hashString(name->start,name->length));
    if(string == NULL){
        string = malloc(sizeof(keyString));
        string->chars = ALLOCATE(char,name->length+1);
        memcpy(string->chars, name->start,name->length);
        string->chars[name->length] = '\0';
        string->length = name->length;
        string->hash = hashString(string->chars,string->length);
    }
    writeArray(temp,string,true);
    return makeConstant(temp);
}

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void binary(bool canAssign) {
    UNUSED(canAssign);
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:          emitByte(OP_ADD); break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
        // case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
        default: return; // Unreachable.
    }
}

static void grouping(bool canAssign) {
    UNUSED(canAssign);
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void parseArray(bool canAssign){
  UNUSED(canAssign);
  const char *arr = parser.previous.start;
  char *valHolder;
  changeArrayDims(currArray,0,currDepth-1);
  if(isDigit(arr[0])||arr[0]=='-'||arr[0]=='.'){
    if(!parseSet){
      currArray->t = DOUBLE_ARR;
    }

    if(currArray->t != DOUBLE_ARR){
      errorAt(&parser.previous,"array is incorrect type for number");
      return;
    }
    double val = strtod(parser.previous.start, &valHolder);
    writeArray(currArray,&val,!parseSet);
    while (valHolder != parser.previous.start+parser.previous.length)
    {
      valHolder++;
      val = strtod(valHolder, &valHolder);
      writeArray(currArray,&val,!parseSet);
    }
  }else if(isAlpha(arr[0])){
    if(currArray->dims.values[currDepth-1] == 0){
      currArray->t = KEY_ARR;
    }

    if(currArray->t != KEY_ARR){
      errorAt(&parser.previous,"array is incorrect type for character");
      return;
    }
    keyString* string = tableFindKey(&currVM->strings,parser.previous.start,
                                            parser.previous.length,hashString(parser.previous.start,parser.previous.length));
    if(string == NULL){
        string = malloc(sizeof(keyString));
        string->chars = ALLOCATE(char,parser.previous.length+1);
        memcpy(string->chars, parser.previous.start,parser.previous.length);
        string->chars[parser.previous.length] = '\0';
        string->length = parser.previous.length;
        string->hash = hashString(string->chars,string->length);
    }
    writeArray(currArray,string,!parseSet);
  }
  parseSet = true;
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t arg = identifierConstant(&name);
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_GLOBAL, arg);
  } else {
    emitBytes(OP_GET_GLOBAL, arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous,canAssign);
}

static void unary(bool canAssign) {
    UNUSED(canAssign);
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable.
    }
}


static void openArray(bool canAssign){
  UNUSED(canAssign);
  currDepth++;  
  parsePrecedence(PREC_ASSIGNMENT);   
}

static void closeArray(bool canAssign){
  UNUSED(canAssign);
  if(currDepth > 1)
    changeArrayDims(currArray,1,currDepth-2);
  shallowestClosedDepth = currDepth < shallowestClosedDepth ? currDepth : shallowestClosedDepth;
  
  currDepth--;
  if(currDepth == 0){
    parseSet = false;
    shallowestClosedDepth = INT_MAX;
    emitConstant(currArray);
    currArray = malloc(sizeof(Array));
    initArray(currArray,UNKNOWN_ARR);
  }
}

static void chain(bool canAssign){
  UNUSED(canAssign);
  parsePrecedence(PREC_ASSIGNMENT);
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_SQUARE]   = {openArray, NULL,  PREC_NONE},
    [TOKEN_RIGHT_SQUARE]  = {NULL, closeArray, PREC_ASSIGNMENT},
    [TOKEN_COMMA]         = {NULL,     chain,  PREC_ASSIGNMENT},
    [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMI]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FORWARD_SLASH] = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IDENTIFIER]    = {variable,     NULL,   PREC_NONE},
    [TOKEN_ARRAY]         = {parseArray, NULL, PREC_ASSIGNMENT},
    //[TOKEN_STRING]      = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {NULL,   NULL,     PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    //[TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    //[TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    //[TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    //[TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    //[TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    //[TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    //[TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
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

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMI,
          "Expect ';' after variable declaration.");

  defineVariable(global);
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMI, "Expect ';' after expression.");
  emitByte(OP_POP);
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMI, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMI) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_BANG:
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

static void statement() {
  if (match(TOKEN_BANG)) {
    printStatement();
  }else{
      expressionStatement();
  }
}

bool compile(VM* vm,const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;

    currVM = vm;

    parser.hadError = false;
    parser.panicMode = false;
    currArray = malloc(sizeof(Array));
    initArray(currArray,UNKNOWN_ARR);

    advance();
    while(!match(TOKEN_EOF)){
        declaration();
    }
    endCompiler();
    return !parser.hadError;
//   #ifdef DEBUG_TOKEN_CREATION
//   int line = -1;
//   for (;;) {
//     Token token = scanToken();
//     if (token.line != line) {
//       printf("%4d ", token.line);
//       line = token.line;
//     } else {
//       printf("   | ");
//     }
//     printf("%2d '%.*s'\n", token.type, token.length, token.start); 

//     if (token.type == TOKEN_EOF) break;
//   }
//   #endif
}