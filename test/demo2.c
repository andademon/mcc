char *a() { return "hello"; }
char *b() { return "world"; }

int main() {
    char *(*array[2])();
    int number = 5;
    array[0] = a;
    array[1] = b;
    printf("%s %s\n", array[0](), array[1]());
    switch(number) {
        case 1: {printf("in case 1\n");break;}
        case 5: {printf("in case 5\n");break;}
        case 10: {printf("in case 10\n");break;}
    }
    return 0;
}