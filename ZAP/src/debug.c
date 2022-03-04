#include <stdio.h>

#include "debug.h"

void printToken(Token t){
    switch (t.type)
    {
    case TOKEN_AMP:
        printf("%10s|%.*s\n", "AMP",t.length, t.start);
        break;
    case TOKEN_IDENTIFIER:
        printf("%10s|%.*s\n", "IDENT",t.length, t.start);
        break;
    case TOKEN_CLASS:
        printf("%10s|%.*s\n", "CLASS",t.length, t.start);
        break;
    default:
        printf("%10s|%.*s\n", "UNKNOWN",t.length, t.start);
        break;
    }
}