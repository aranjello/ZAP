#pragma once

#include <string>

using std::string;

typedef enum
{
    //Array input tokens
    TOKEN_OPEN_SQUARE, TOKEN_CLOSED_SQUARE, TOKEN_COMMA, TOKEN_NUMBER, TOKEN_CHARACTER,
    //modifier tokens
    TOKEN_BANG, TOKEN_AT, TOKEN_AMP, TOKEN_DOLLAR,
    //identifier tokens
    TOKEN_IDENTIFIER,
    // control tokens
    TOKEN_SEMICOLON,
    //test tokens
    TOKEN_A,
    TOKEN_B,
    TOKEN_C,
    TOKEN_UNKNOWN,
} tokenType;

class token{
    public:
        tokenType type;
        string content;
        int line;
        int linePos;
        token(tokenType t, string c, int l, int lp);
};