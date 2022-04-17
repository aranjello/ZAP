#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

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
    string input;
    bool fileRead = false;
    if(argc > 1){
        for (int i = 1; i < argc;i++){
            if(argv[i][0] == '-'){
                switch (argv[i][i])
                {
                case 'd':
                    break;
                
                default:
                    break;
                }
            }else{
                if(fileRead){
                    printf("Usage: ./ZAP {optional file to run} {optional flag1, optional flag2, ...}");
                    return 1;
                }
                std::ifstream t(argv[i]);
                if(!t.good()){
                    printf("Could not open file %s\n",argv[i]);
                    printf("Usage: ./ZAP {optional file to run} {optional flag1, optional flag2, ...}");
                    return 1;
                }
                std::stringstream buffer;
                buffer << t.rdbuf();
                input = buffer.str();
                t.close();
                buffer.clear();
                parseLine(input);
                fileRead = true;
            }
        }
    }
    if(!fileRead){
        std::cout << ">";
        std::getline(std::cin, input);
        while(parseLine(input) != PARSE_EXIT){
            std::cout << input << std::endl << ">";
            std::getline(std::cin, input);
        }
    }
    return 0;
}