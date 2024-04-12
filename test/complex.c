#include <stdio.h>

char *a() { return "hello"; }
char *b() { return "world"; }
char *(*c())() { return a; }
char *(*d())() { return b; }

char *(*(*(*e())[])())() {
    static char *(*(*array[])())() = {c, d};
    return &array;
}

int main() {
    char *(*(*(*hello_world)[])())() = e();
    printf("%s %s\n", (*hello_world)[0]()(), (*hello_world)[1]()());
    return 0;
}