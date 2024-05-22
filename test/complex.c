
char *a() { return "hello"; }
char *b() { return "world"; }
char *(*c())() { return a; }
char *(*d())() { return b; }

char *(*(*array[2])())();

char *(*(*(*e())[])())() {
    array[0] = c;
    array[1] = d;
    return &array;
}

int main() {
    char *(*(*(*hello_world)[2])())() = e();
    printf("%s %s\n", (*hello_world)[0]()(), (*hello_world)[1]()());
    return 0;
}