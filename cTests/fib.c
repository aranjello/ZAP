#include <stdio.h>

static int fib(int n){
    return n < 2?n:fib(n-1)+fib(n-2);
}

int main(int argc, const char* argv[]) {
    int val = 40;
    printf("fib of %d is %d\n",val,fib(val));
}