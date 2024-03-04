char a;
char b;
char c;

int sub(int a, int b) {
    return a + b;
}

int main(void) {
    int a;
    int b;
    a = 1;
    b = 2;
    (a + b) * 3 + (a - b);
    sub(a, b);
    if (a == 10) {
        a = a - 1;
    }
    else if(a == 0) {
        a = a + 1;
    }
    else if (a == 3) {
        b = b - 1;
    }
    else {
        sub(b, a);
    }
    return a + b;
}