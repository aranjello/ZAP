#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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
     PREC_ASSIGNMENT, // = []
     PREC_OR, // or
     PREC_AND, // and
     PREC_EQUALITY, // == !=
     PREC_COMPARISON, // < > <= >=
     PREC_ARRAY,
     PREC_TERM, // + -
     PREC_FACTOR, // * /
     PREC_UNARY, // ! -
     PREC_CALL, // . ()
     PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;

Array* currArray;

Parser parser;

Compiler* current = NULL;

int currDepth = 0;

int shallowestClosedDepth = INT_MAX;

bool parseSet = false;

Chunk* compilingChunk;

VM* currVm;

/*
get the current chunk we are working in
@return The current chunk
*/
static Chunk* currentChunk() {
  return compilingChunk;
}

static void initCompiler(Compiler* compiler) {
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  current = compiler;
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
  int constant = addConstantArray(currArray).offset;
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  currArray = initEmptyArray(VAL_UNKNOWN);
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

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
}

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable() {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;

  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; 
    }

    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }

  addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  declareVariable();
  if (current->scopeDepth > 0) return 0;
  uint32_t hash = hashString(parser.previous.start, parser.previous.length);
  po p = tableFindKey(&currVm->globalInterned,parser.previous.start, parser.previous.length, hash);
  if(p.ptr == NULL){
    p = addGlobKey(parser.previous.start,parser.previous.length);
    tableSet(&currVm->globalInterned, p.ptr, NULL);
  }
  
  return (uint8_t)p.offset;
}

static void markInitialized() {
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}

static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void binary(bool canAssign) {
  UNUSED(canAssign);
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_PLUS:          emitByte(OP_ADD);          break;
    case TOKEN_APPEND:        emitByte(OP_PUSH_TO_ARR);  break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT);     break;
    case TOKEN_REMOVE:        emitByte(OP_POP_FROM_ARR); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY);     break;
    case TOKEN_FORWARD_SLASH: emitByte(OP_DIVIDE);       break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_COMPARE);      break;
    case TOKEN_GREATER:       emitByte(OP_GREATER);      break;
    case TOKEN_LESS:          emitByte(OP_LESS);         break;
    case TOKEN_DOT:           emitByte(OP_DOT_PROD);     break;

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
    po p = addConstantArray(initEmptyArray(VAL_DOUBLE));
    int constant = p.offset;
    if (constant > UINT8_MAX) {
      error("Too many constants in one chunk.");
      return;
    }
    
    emitBytes(OP_ARRAY, constant);
  }
  consume(TOKEN_SEMI, "expected ';'1");
  defineVariable(global);
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMI, "expected ';'2");
  emitByte(OP_POP);
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMI, "expected semicolon\n");
  emitByte(OP_PRINT);
}

static void getValTypeStatement() {
  expression();
  emitByte(OP_GET_TYPE);
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_ARRAY:
      case TOKEN_SEMI:
          return;

          default:; // Do nothing.
    }

    advance();
  }
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;
  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
    emitByte(OP_POP);
    current->localCount--;
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
  consume(TOKEN_LEFT_PAREN, "expect ( before condition in if\n");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition in if.");

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
  consume(TOKEN_LEFT_PAREN, "')' opens while");
  int loopStart = currentChunk()->count;
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);
  patchJump(exitJump);
  emitByte(OP_POP);
}

static void forStatement() {
  beginScope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMI)) {
    // No initializer.
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }
  // consume(TOKEN_SEMI, "Expect ';' after first for.");

  int loopStart = currentChunk()->count;
  int exitJump = -1;
  if (!match(TOKEN_SEMI)) {
    expression();
    consume(TOKEN_SEMI, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

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
  endScope();
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void statement() {
  if (match(TOKEN_BANG)) {
    printStatement();
  } else if(match(TOKEN_AT)){
    getValTypeStatement();
  }else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  }else if (match(TOKEN_QUESTION)) {
    ifStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  }else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else {
    expressionStatement();
  }
  //consume(TOKEN_SEMI, "expected semicolon\n");
}

static void grouping(bool canAssign){
  UNUSED(canAssign);
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void lookUpGrouping(bool canAssign){
  UNUSED(canAssign);
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
  emitByte(OP_LOOKUP);
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

static bool foundDecimal(Token t){
  const char *tokenIterartor = t.start;
  for (; tokenIterartor != t.start + t.length;tokenIterartor++)
  {
    if(*tokenIterartor == '.')
      return true;
  }
  return false;
}

static bool hasChars(Token t){
  const char *tokenIterartor = t.start;
  for (; tokenIterartor != t.start + t.length;tokenIterartor++)
  {
    if(isAlpha(*tokenIterartor))
      return true;
  }
  return false;
}

static void parseArray(bool canAssign){
  UNUSED(canAssign);  
  const char *arr = parser.previous.start;
  char *valHolder;
  changeArrayDims(currArray,0,currDepth-1);
  if(!hasChars(parser.previous)){
    ValueType numType = foundDecimal(parser.previous) ? VAL_DOUBLE : VAL_INT;
    // printf("numType is %s", numType == VAL_INT ? "VAL_INT" : "VAL_DOUBLE");
    if(!parseSet){
      currArray->type = numType;
    }

    if(currArray->type != numType){
      errorAt(&parser.previous,"array is incorrect type for number");
      return;
    }
    double val = strtod(parser.previous.start, &valHolder);
    int intVal = val;
    void *valPtr;
    if(numType == VAL_INT)
      valPtr = &intVal;
    else
      valPtr = &val;
    createNewVal(currArray,valPtr,!parseSet);
    while (valHolder != parser.previous.start+parser.previous.length)
    {
      valHolder++;
      val = strtod(valHolder, &valHolder);
      intVal = val;
      void *valPtr;
      if(numType == VAL_INT)
        valPtr = &intVal;
      else
        valPtr = &val;
      createNewVal(currArray,valPtr,!parseSet);
    }
  }else{
    if(currArray->dims.values[currDepth-1] == 0){
      currArray->type = VAL_CHAR;
    }

    if(currArray->type != VAL_CHAR){
      errorAt(&parser.previous,"array is incorrect type for character");
      return;
    }
    while (arr != parser.previous.start+parser.previous.length)
    {
      createNewVal(currArray, (char*)&arr[0],!parseSet);
      arr++;
    }
    char close = '\0';
    createNewVal(currArray, &close,!parseSet);
    currArray->hash = hashString(currArray->as.chars,currArray->count);
  }
  parseSet = true;
  // if(currDepth == 1)
  //emitArray();
  //advance();
}


static void unary(bool canAssign) {
  UNUSED(canAssign);
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
    case TOKEN_BAR:
      emitByte(OP_ANY);
      break;
    default: return; // Unreachable.
  }
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static void namedVariable(Token name, bool canAssign) {

  po p;
  uint8_t getOp, setOp;
  p.offset = resolveLocal(current, &name);
  if (p.offset != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else {
    uint32_t hash = hashString(name.start, name.length);
    p = tableFindKey(&currVm->globalInterned, name.start, name.length, hash);
    if(p.ptr == NULL){
      p = addGlobKey(parser.previous.start,parser.previous.length);
      tableSet(&currVm->globalInterned, p.ptr, NULL);
    };
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, p.offset);
  } else {
    emitBytes(getOp, p.offset);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void or_(bool canAssign) {
  UNUSED(canAssign);
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void and_(bool canAssign) {
  UNUSED(canAssign);
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

static void chain(bool canAssign);

static void createMultiDim(bool canAssign){
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
    emitArray();
  }
}


static void lookup(bool canAssign){
  printf("attempting lookup\n");
  createMultiDim(canAssign);
  emitByte(OP_LOOKUP);
}

static void chain(bool canAssign){
  UNUSED(canAssign);
  parsePrecedence(PREC_ASSIGNMENT);
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping,       lookUpGrouping, PREC_ASSIGNMENT},
    [TOKEN_RIGHT_PAREN]   = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_LEFT_BRACE]    = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_RIGHT_BRACE]   = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_ARRAY]         = {parseArray,     NULL,           PREC_ARRAY     },
    [TOKEN_LEFT_SQUARE]   = {createMultiDim, lookup,         PREC_ASSIGNMENT},
    [TOKEN_RIGHT_SQUARE]  = {NULL,           closeArray,     PREC_ASSIGNMENT},
    [TOKEN_COMMA]         = {NULL,           chain,          PREC_ASSIGNMENT},
    [TOKEN_DOT]           = {NULL,           binary,         PREC_FACTOR    },
    [TOKEN_MINUS]         = {unary,          binary,         PREC_TERM      },
    [TOKEN_REMOVE]        = {NULL,           binary,         PREC_ASSIGNMENT},
    [TOKEN_PLUS]          = {unary,          binary,         PREC_TERM      },
    [TOKEN_APPEND]        = {NULL,           binary,         PREC_ASSIGNMENT},
    [TOKEN_SEMI]          = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_FORWARD_SLASH] = {NULL,           binary,         PREC_FACTOR    },
    [TOKEN_STAR]          = {NULL,           binary,         PREC_FACTOR    },
    [TOKEN_AMP]           = {unary,          NULL,           PREC_NONE      },
    [TOKEN_BAR]           = {unary,          NULL,           PREC_NONE      },
    [TOKEN_BANG]          = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_BANG_EQUAL]    = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_EQUAL]         = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_EQUAL_EQUAL]   = {NULL,           binary,         PREC_EQUALITY  },
    [TOKEN_GREATER]       = {NULL,           binary,         PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_LESS]          = {NULL,           binary,         PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_IDENTIFIER]    = {variable,       NULL,           PREC_NONE      },
    [TOKEN_POUND]         = {unary,          NULL,           PREC_NONE      },
    //[TOKEN_STRING]        = {NULL,         NULL,           PREC_NONE      },
    [TOKEN_NUMBER]        = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_CHAR]          = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_AND]           = {NULL,           and_,           PREC_NONE      },
    [TOKEN_CLASS]         = {NULL,           NULL,           PREC_NONE      },
    //[TOKEN_ELSE]          = {NULL,         NULL,           PREC_NONE      },
    [TOKEN_FALSE]         = {NULL,           NULL,           PREC_NONE      },
    //[TOKEN_FOR]           = {NULL,         NULL,           PREC_NONE      },
    [TOKEN_FUN]           = {NULL,           NULL,           PREC_NONE      },
    //[TOKEN_IF]            = {NULL,         NULL,           PREC_NONE      },
    //[TOKEN_NIL]           = {NULL,         NULL,           PREC_NONE      },
      [TOKEN_OR]          = {NULL,           or_,            PREC_NONE      },
    //[TOKEN_PRINT]         = {NULL,         NULL,           PREC_NONE      },
    //[TOKEN_RETURN]        = {NULL,         NULL,           PREC_NONE      },
    //[TOKEN_SUPER]         = {NULL,         NULL,           PREC_NONE      },
    //[TOKEN_THIS]          = {NULL,         NULL,           PREC_NONE      },
    [TOKEN_TRUE]          = {NULL,           NULL,           PREC_NONE      },
    //[TOKEN_VAR]           = {NULL,         NULL,           PREC_NONE      },
    //[TOKEN_WHILE]         = {NULL,         NULL,           PREC_NONE      },
    [TOKEN_ERROR]         = {NULL,           NULL,           PREC_NONE      },
    [TOKEN_EOF]           = {NULL,           NULL,           PREC_NONE      },
};

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <=     PREC_ASSIGNMENT;
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

bool compile(VM* vm,const char* source, Chunk* chunk) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler);
  currVm = vm;
  compilingChunk = chunk;
  parser.hadError = false;
  parser.panicMode = false;
  currArray = initEmptyArray(VAL_UNKNOWN);
  advance();
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}
