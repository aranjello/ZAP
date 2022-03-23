#include <stdarg.h>

#include <stdio.h>

#include "tokenParser.h"
#include "memUtils.h"

bool panicMode = false;

void initAST(const char* source){
    initScanner(source);
}

static void addToken(statement* s, Token inToken){
    if(s->tokenCount + 1 > s->tokenCapacity){
        s->tokenCapacity = arrCapSize(s->tokenCapacity);
        s->tokens = (Token*)resizeArr(s->tokens, s->tokenCapacity * sizeof(Token));
    }
    s->tokens[s->tokenCount] = inToken;
    s->tokenCount++;
}

//peek: lookahead some number of tokens
static Token peek(int lookAhead){
    Token initalToken = scanToken();
    for (int i = 0; i < lookAhead-1; i++){
        scanToken();
    }
    Token t = scanToken();
    rewindScanner(initalToken);
    return lookAhead > 0 ? t : initalToken;
}
//match: return true if is token type and consume and add to parser, return false and do nothing otherwise
static bool match(TokenType check, statement* s){
    Token t = scanToken();
    printf("matching token %.*s\n", t.length, t.start);
    if(t.type == check){
        addToken(s, t);
        return true;
    }
    rewindScanner(t);
    return false;
}
//rewind: remove last token from parser and rewind token finder to its tokens start
static void rewindStatement(statement* s){
    s->tokenCount -= 1;
    rewindScanner(s->tokens[s->tokenCount]);
}

static bool scanOrder(statement* s, int numTokens, ...){
    va_list tokens;
    va_start(tokens, numTokens);
    for (int i = 0; i < numTokens; i++){
        TokenType tokeType = va_arg(tokens, TokenType);
        Token t = scanToken();
        // printf("Token type is %d and token is %.*s type %d \n",tokeType,t.length,t.start,t.type);
        addToken(s, t);
        if(t.type != tokeType){
            s->tokenCount = s->tokenCount - i - 1;
            rewindScanner(s->tokens[s->tokenCount]);
            return false;
        }
    }
    va_end(tokens);
    return true;
}

void panicError(statement* s){
    Token t = scanToken();
    while(t.type != TOKEN_SEMI){
        addToken(s, t);
        t = scanToken();
    }
    addToken(s, t);
}


static bool isDigit(statement* s){
    //Token t = scanToken();
    if(match(TOKEN_NUMBER,s)){
        return true;
    }
    return false;
}

static bool isNumVal(statement* s){
    if(isDigit(s)){
        if(match(TOKEN_COMMA,s)){
            if(isNumVal(s)){
                return true;
            }else{
                rewindStatement(s);
                return false;
            }
        }else{
            return true;
        }
    }
    return false;
}

static bool isArr(statement* s);

static bool isArrInt(statement* s){
    if(match(TOKEN_ARRAY,s)){
        return true;
    }
    if(isArr(s)){
        if(match(TOKEN_COMMA,s)){
            if(isArrInt(s)){
                return true;
            }else{
                rewindStatement(s);
                return false;
            }
        }else{
            return true;
        }
    }
    rewindStatement(s);
    return false;
}

static bool isArr(statement* s){
    if(match(TOKEN_LEFT_SQUARE,s)){
        if(isArrInt(s)){
            if(match(TOKEN_RIGHT_SQUARE,s)){
                return true;
            }else{
                rewindStatement(s);
                return false;
            }
        }else{
            rewindStatement(s);
            return false;
        }
    }
    rewindStatement(s);
    return false;
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
    //printf("token in ident is %.*s\n", t.length, t.start);
    if(match(TOKEN_SEMI,&s)){
        s.type = VAR_REF;
        return s;
    }
    if(match(TOKEN_EQUAL,&s)){
        if(match(TOKEN_IDENTIFIER,&s)){
            if(match(TOKEN_SEMI,&s)){
                s.type = VAR_DEF;
            }
            return s;
        }
        if(isArr(&s)){
            if(match(TOKEN_SEMI,&s)){
                s.type = VAR_DEF;
                return s;
            }else{
                rewindStatement(&s);
            }
        }
    }
   
    // if(
    //     scanOrder(&s,5,TOKEN_EQUAL,TOKEN_LEFT_SQUARE,TOKEN_ARRAY,TOKEN_RIGHT_SQUARE,TOKEN_SEMI)
    //     || scanOrder(&s,3,TOKEN_EQUAL,TOKEN_IDENTIFIER,TOKEN_SEMI)
    //     || scanOrder(&s,1,TOKEN_SEMI)
    //     || scanOrder(&s,5,TOKEN_EQUAL,TOKEN_LEFT_SQUARE,TOKEN_ARRAY,TOKEN_RIGHT_SQUARE,TOKEN_SEMI)){
    //     s.type = VAR_DEF;
    //     return s;
    // }
    // if(scanOrder(&s,1,TOKEN_SEMI)){
    //     s.type = VAR_REF;
    //     return s;
    // }
    panicError(&s);
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
printf("token in start is %.*s type is %d\n", curr.length, curr.start,curr.type);
    if(curr.type == TOKEN_IDENTIFIER)
        return identStatement(curr);
    if(curr.type == TOKEN_EOF){
        s.type = FIN_PARSE;
        return s;
    }
    return s;
}