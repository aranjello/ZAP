#include <stdio.h>
#include <string.h>

int maint(int argc, const char* argv[]) {
    char str[10];
    strcpy(str, "str");
    printf("%s", strcmp(strcat(str,"ing"),"string")==0 ? "t":"f");
}