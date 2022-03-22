#include <stdarg.h>

#include <stdio.h>

#include "tokenParser.h"
#include "memUtils.h"

void initAST(const char* source){
    initScanner(source);
}

static void addToken(statement* s, Token inToken){
    if(s->tokenCount + 1 > s->tokenCapacity){
        s->tokenCapacity = arrCapSize(s->tokenCapacity);
        s->tokens = (Token*)resizeArr(s->tokens, s->tokenCapacity * sizeof(Token));
    }
    s->tokens[s->tokenCount] = inToken;
    // outToken.length = inToken.length;
    // printf("here6\n");
    // outToken.line = inToken.line;
    // outToken.start = inToken.start;
    // outToken.type = inToken.type;
    s->tokenCount++;
}

static bool scanOrder(statement* s, int numTokens, ...){
    
    va_list tokens;
    va_start(tokens, numTokens);
    for (int i = 0; i < numTokens; i++){
        TokenType tokeType = va_arg(tokens, TokenType);
        Token t = scanToken();
        printf("Token type is %d and token is %.*s type %d \n",tokeType,t.length,t.start,t.type);
        addToken(s, t);
        if(t.type != tokeType){
            s->tokenCount = s->tokenCount - i - 1;
            rewindScanner(s->tokens[s->tokenCount].start);
            return false;
        }
    }
    va_end(tokens);
    return true;
}

static statement identStatement(Token t){
    statement s;
    s.tokenCapacity = 0;
    s.tokenCount = 0;
    s.tokens = NULL;
    s.numParams = 0;
    s.returns = false;
    s.type = ERROR;
    addToken(&s, t);
    if(scanOrder(&s,5,TOKEN_EQUAL,TOKEN_LEFT_SQUARE,TOKEN_ARRAY,TOKEN_RIGHT_SQUARE,TOKEN_SEMI)){
        s.type = VAR_DEF;
        return s;
    }
    if(scanOrder(&s,1,TOKEN_SEMI)){
        s.type = VAR_REF;
        return s;
    }
    return s;
}

statement scanStatement(){
    Token curr = scanToken();
    statement s;
    s.tokenCapacity = 0;
    s.tokenCount = 0;
    s.tokens = NULL;
    s.numParams = 0;
    s.returns = false;
    s.type = ERROR;
    if(curr.type == TOKEN_IDENTIFIER)
        return identStatement(curr);
    if(curr.type == TOKEN_EOF){
        s.type = FIN_PARSE;
        return s;
    }
    return s;
}