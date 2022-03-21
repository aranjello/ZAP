#include <stdio.h>

int main(int argc, const char* argv[]) {
    int sum = 0;
    for(int i = 100000; i != 0; i--){
        sum += i;
    }
    printf("%d\n",sum);
}