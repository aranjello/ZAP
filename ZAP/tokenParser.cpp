#include <ZAP/tokenParser.hpp>
#include <ZAP/utils.hpp>


vector<token> tokenList;

static string currString;

static bool noneFound = true;

static int arrDepth = 0;
static int currLine = 0;
static int currLinePos = 0;

static void clearWhiteSpace(size_t* index){
    char currChar = currString.at(*index);
    while(currChar == ' ' || currChar == '\n' || currChar == '\r'){
        currLinePos++;
        if(currChar == '\n'){
            currLine++;
            currLinePos = 0;
        }
        *index += 1;
        if(*index > currString.length())
            break;
        currChar = currString.at(*index);
    }
}

static bool matchAndAdd(size_t* initialIndex, string match, tokenType t){
    if(*initialIndex + match.length() > currString.length())
        return false;
    if(currString.substr(*initialIndex,match.length()) == match){
        tokenList.push_back(token(t, match, currLine, currLinePos));
        currLinePos += match.length();
        *initialIndex += match.length();
        noneFound = false;
        return true;
    }
    return false;
}

static void matchAndAddNum(size_t* initialIndex){
    size_t endIndex = *initialIndex;
    while(endIndex < currString.length() && isNumber(currString.at(endIndex))){
        endIndex++;
    }
    string identifier = currString.substr(*initialIndex, (endIndex - *initialIndex));
    tokenList.push_back(token(TOKEN_NUMBER, identifier, currLine, currLinePos));
    currLinePos += identifier.length();
    *initialIndex += identifier.length();
    noneFound = false;
}

static void matchAndAddIdentifier(size_t* initialIndex){
    size_t endIndex = *initialIndex;
    while(endIndex < currString.length() && isAlphanumeric(currString.at(endIndex))){
        endIndex++;
    }
    string identifier = currString.substr(*initialIndex, (endIndex - *initialIndex));
    tokenList.push_back(token(TOKEN_IDENTIFIER, identifier, currLine, currLinePos));
    currLinePos += identifier.length();
    *initialIndex += identifier.length();
    noneFound = false;
}

//longest match must always come first
//e.g ![ must match before ! or [
vector<token> getTokens(string input){
    currString = input;
    
    for (size_t i = 0; i < input.length();){
        clearWhiteSpace(&i);
        noneFound = true;
        matchAndAdd(&i, "[", TOKEN_OPEN_SQUARE);
        matchAndAdd(&i, "]", TOKEN_CLOSED_SQUARE);
        matchAndAdd(&i, ",", TOKEN_COMMA);
        if (matchAndAdd(&i, ";", TOKEN_SEMICOLON)){
            tokenList.clear();
        }
        if (noneFound)
        {
            char currChar = input.at(i);
            if (isNumber(currChar) || currChar == '-' || currChar == '.')
            {
                matchAndAddNum(&i);
            }
            else if (isCharacter(currChar))
            {
                matchAndAddIdentifier(&i);
            }
            else
            {
                tokenList.push_back(token(TOKEN_UNKNOWN, currChar + "", currLine, currLinePos));
                currLinePos++;
                i++;
            }
        }
    }
    return tokenList;
}