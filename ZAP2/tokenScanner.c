#include <stdio.h>
#include <string.h>

#include "common.h"
#include "tokenScanner.h"

int arrayDepth = 0;

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

Scanner scanner;
/*
reset scanner vars and point scanner to start of input text
@param source the source text for the scanner to scan
*/
void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

/*
Return the char at current and increment current
@return The char at current
*/
static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

/*
return the char at current without incrementing
@return The char at current
*/
static char peek() {
  return *scanner.current;
}

/*
check to see if current is at the end of the input text
@return Whether the current char is equal to \0
*/
static bool isAtEnd() {
  return *scanner.current == '\0';
}

/*
return the char after current but dont increment
@return The char after current
*/
static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}

/*
check the char at current and return whether it is the expected or not.
Increment if expected.
@param expected The char we want to match against
@return Whether the current char matches the expected char
*/
static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;
  scanner.current++;
  return true;
}

/*
Check if the input char is a letter
@param c The char to check
@return c is a letter
*/
static bool isAlpha(char c) {
    return (c >= 'a'       && c <= 'z') ||
           (c >= 'A'       && c <= 'Z') ||
           (arrayDepth > 0 && c == ' ') || 
           c == '_';
}

/*
Check if the input char is a digit
@param c The char to check
@return c is a digit
*/
static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

/*
Create a token from the input currently being scanned
@param type The type of the token being scanned
@return The created token
*/
static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

/*
Creates a special token with type error and content of some arbitrary message
@param message The error message
@return An error token
*/
static Token errorToken(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

/*
Skips all whitespace before the start of a token
*/
static void skipWhitespace() {
    // if(arrayDepth > 0)
    //     return;
    for (;;)
    {
        char c = peek();
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        case '/':
            if (peekNext() == '/')
            {
                // A comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd())
                    advance();
            }
            else
            {
                return;
            }
            break;
        default:
            return;
        }
  }
}

/*
Checks currently scanned token to verify if it is a keyword
@param start The number of letters into a keyword to start checking from
@param length The length of the keyword characters left to check
@param rest The rest of the characters to check for a keyword
@param type The token type to return if the currently scanned token is a keyword
@return The type of the currently scanned token
*/
static TokenType checkKeyword(int start, int length,
    const char* rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

/*
checks currently scanned token to see if it is possibly a keyword
@return The type of token that the currently scanned token is
*/
static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'f':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
            }
        }
        break;
        case 'n': return checkKeyword(1, 3, "ull", TOKEN_NULL);
        case 't': return checkKeyword(1, 3, "rue", TOKEN_TRUE);
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
    }
  return TOKEN_IDENTIFIER;
}

/*
Creates a token from potential identifier or keyword
@return The newly created token
*/
static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

/*
Creates a token for a number
@return The newly creaetd token
*/
// static Token number() {
//   while (isDigit(peek())) advance();

//   // Look for a fractional part.
//   if (peek() == '.' && isDigit(peekNext())) {
//     // Consume the ".".
//     advance();

//     while (isDigit(peek())) advance();
//   }

//   return makeToken(TOKEN_NUMBER);
// }

static Token arrayInterior(){
  if(arrayDepth == 0)
    return errorToken("array interior must be inside []");
  while(peek() != ']'){
    advance();
  }
  return makeToken(TOKEN_ARRAY);
}

/*
Creates a token for a string
@return The newly created string token
*/
// static Token string() {
//   while (peek() != '"' && !isAtEnd()) {
//     if (peek() == '\n') scanner.line++;
//     advance();
//   }

//   if (isAtEnd()) return errorToken("Unterminated string.");

//   // The closing quote.
//   advance();
//   return makeToken(TOKEN_STRING);
// }

/*
scans the current input text for the next token available
@return The next token available
*/
Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);
    char c = advance();
    if (isAlpha(c)) return (arrayDepth == 0)? identifier():arrayInterior();
    if (isDigit(c)) return arrayInterior();

    switch (c) {
        case '(':
        return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '[':
          arrayDepth++;
          return makeToken(TOKEN_LEFT_SQUARE);
        case ']':
          arrayDepth--;
          return makeToken(TOKEN_RIGHT_SQUARE);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMI);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return (arrayDepth == 0)?makeToken(TOKEN_DOT):arrayInterior();
        case '-': return (arrayDepth == 0)?makeToken(TOKEN_MINUS):arrayInterior();
        case '#': return makeToken(TOKEN_POUND);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_FORWARD_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '@': return makeToken(TOKEN_AT);
        case '?': return makeToken(
          match('!')?TOKEN_WHILE:match('?')?TOKEN_FOR:TOKEN_QUESTION);
        case '&':
          return makeToken(
              match('&') ? TOKEN_AND : TOKEN_AMP);
        case '|':
          return makeToken(
              match('|') ? TOKEN_OR : TOKEN_BAR);
        case '!':
        return makeToken(
            match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
        return makeToken(
            match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
        return makeToken(
            match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
        return makeToken(
            match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        // case '"': return string();
    }
    return errorToken("Unexpected character.");
}