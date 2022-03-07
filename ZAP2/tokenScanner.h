#ifndef zap_scanner_h
#define zap_scanner_h

typedef enum {
    //Math tokens
    TOKEN_PLUS,TOKEN_MINUS,TOKEN_STAR,TOKEN_FORWARD_SLASH,TOKEN_PERCENT,
    TOKEN_CARRET,
    //modified tokens
    TOKEN_VAR,TOKEN_BAR,TOKEN_AND,TOKEN_OR,TOKEN_WHILE,TOKEN_FOR,
    //comparison tokens
    TOKEN_LESS,TOKEN_GREATER,TOKEN_EQUAL,TOKEN_LESS_EQUAL,TOKEN_GREATER_EQUAL,
    TOKEN_BANG_EQUAL,TOKEN_EQUAL_EQUAL,
    //function tokens
    TOKEN_POUND,TOKEN_DOLLAR,
    TOKEN_BANG,TOKEN_AT,
    //control tokens
    TOKEN_QUESTION,TOKEN_SEMI,TOKEN_AMP,
    //Grouping tokens
    TOKEN_LEFT_PAREN,TOKEN_RIGHT_PAREN,TOKEN_LEFT_SQUARE,TOKEN_RIGHT_SQUARE,
    TOKEN_LEFT_BRACE,TOKEN_RIGHT_BRACE,TOKEN_ARRAY,
    //Array elems
    TOKEN_NUMBER,TOKEN_CHAR,TOKEN_TRUE,TOKEN_FALSE,TOKEN_NULL,TOKEN_COMMA,
    TOKEN_DOT,
    //object tokens
    TOKEN_IDENTIFIER,TOKEN_CLASS,TOKEN_FUN,
    //clean up tokens
    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void initScanner(const char* source);

Token scanToken();

#endif