#include <stdio.h>

#include "common.h"
#include "tokenParser.h"

int main(int argc, const char* argv[]){
    UNUSED(argc);
    UNUSED(argv);
    initAST("test = [1,2,3];\
             test =;");
    statement s = scanStatement();
    while(s.type != FIN_PARSE){
        printf("stament is type %d\n", s.type);
        for (int i = 0; i < s.tokenCount; i++){
            printf("Token %d is %.*s\n", i, s.tokens[i].length, s.tokens[i].start);
        }
            s = scanStatement();
    }
    printf("running\n");
}