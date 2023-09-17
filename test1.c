#include <stdlib.h>
#include <stdio.h>
#include <string.h>



int main(int argc, char* argv[]) {
    // // int value;
    // // scanf("%x", &value);
    // // printf("%d\n",value);
    // short* cache;
    // //  = (short*) calloc(1, sizeof(short));
    // char a[5];
    // scanf("%s .", a);
    // printf("%d\n", strcmp(a, "12")==0);
    // printf("%s\n", a);

    char* c = (char*) calloc(1,sizeof(char)*8);
    scanf("%x .", c);
    if (c) {
        printf("%hhx\n",*(c));
    }
    exit(0);
}
