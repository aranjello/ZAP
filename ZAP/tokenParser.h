#pragma once

#include "common.h"
#include "tokenScanner.h"

typedef enum{
    UNKNOWN,ERROR,NAT_FUN,CONT_FLOW,UNARY_OP,BINARY_OP,VAR_DEF,VAR_REF,FUN_DEF,FUN_CALL,FIN_PARSE,
} statmentTypes;

typedef struct {
    statmentTypes type;
    int numParams;
    bool returns;
    int tokenCount;
    int tokenCapacity;
    Token *tokens;
}statement;

void initAST(const char* source);
statement scanStatement();
