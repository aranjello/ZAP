#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
    // for (int i = 0; i < argc; i++){
    //     printf("%s\n", argv[i]);
    // }
    int retCode = system("ZAP.exe test/tests/test.txt > ersult.txt");
    return 0;
}