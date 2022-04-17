#include <string>
#include <iostream>

#include <ZAP\utils.hpp>
#include <ZAP\tokenParser.hpp>
#include <ZAP\debug.hpp>

using std::string;

typedef enum
{
    PARSE_OK,
    PARSE_ERROR,
    PARSE_EXIT
} parseStatus;

parseStatus parseLine(string input){
    if(input == "exit")
        return PARSE_EXIT;
    vector<token> res = getTokens(input);
    #ifdef DEBUG_TOKEN_CREATION
        printTokens(res);
    #endif
    res.clear();
    return PARSE_OK;
}

int main(int argc, char** argv){
    UNUSED(argc);
    UNUSED(argv);
    string input;

    std::cout << ">";
    std::getline(std::cin, input);
    
    while(parseLine(input) != PARSE_EXIT){
        std::cout << input << std::endl << ">";
        std::getline(std::cin, input);
    }
    return 0;
}