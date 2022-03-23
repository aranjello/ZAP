#include <stdio.h>

#include "common.h"
#include "tokenParser.h"

int main(int argc, const char* argv[]){
    UNUSED(argc);
    UNUSED(argv);
    initAST("test = [[1,2,3],[4,5,6],[7,8,9]];    test; test = test;");
    int charCount = 0;
    const char *endOfLastStatement;
    statement s = scanStatement();
    while(s.type != FIN_PARSE){
        while(*endOfLastStatement == ' ' || *endOfLastStatement == '\n'){
            if(*endOfLastStatement != '\n')
                charCount++;
            else
                charCount = 0;
            endOfLastStatement++;
        }
        printf("stament is ");
        if(s.type == ERROR){
            printf("unrecognized at line %d index %d :", s.tokens->line,charCount);
        }
        for (int i = 0; i < s.tokenCount; i++)
        {
            if(i>0)
                printf(" ");
            printf("%.*s", s.tokens[i].length, s.tokens[i].start);
        }
        printf(" type % d\n", s.type);
        endOfLastStatement = (s.tokens[s.tokenCount - 1].start + s.tokens[s.tokenCount - 1].length);
        charCount += endOfLastStatement - s.tokens->start;
        s = scanStatement();
    }
    printf("running\n");
}