#include <stdio.h>
#include <stdlib.h>
#include <string>

int main(int argc, char** argv){
    // for (int i = 0; i < argc; i++){
    //     printf("%s\n", argv[i]);
    // }
    char buff[100];
    printf("%d %s\n", argc, argv[1]);
    sprintf(buff, "%s test/tests/test.ZUT > result.txt", argv[1]);
    printf("%s\n", buff);
    int retCode = system(buff);
    return 0;
}