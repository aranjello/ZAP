#include <iostream>

#include <ZAP\debug.hpp>

void printTokens(std::vector<token> tokenList){
    #define TOKEN_PRINT(NAME,index)   \
        case NAME:              \
            std::cout << #NAME << " line index:" << tokenList.at(index).linePos << " content: " << tokenList.at(index).content; \
            break
    for (size_t i = 0; i < tokenList.size(); i++){
        switch (tokenList.at(i).type)
        {
            TOKEN_PRINT(TOKEN_OPEN_SQUARE,i);
            TOKEN_PRINT(TOKEN_CLOSED_SQUARE,i);
            TOKEN_PRINT(TOKEN_COMMA,i);
            TOKEN_PRINT(TOKEN_UNKNOWN, i);
            TOKEN_PRINT(TOKEN_IDENTIFIER, i);
            TOKEN_PRINT(TOKEN_CHARACTER,i);
            TOKEN_PRINT(TOKEN_NUMBER,i);
        }
        std::cout << std::endl;
    }
    #undef TOKEN_PRINT
}