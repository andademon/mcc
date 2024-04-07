void test(int a,int b) {
    if (a < b) {
        printf("a < b\n");
    }
    else if (a > b) {
        printf("a > b\n");
    }
    else {
        printf("a == b\n");
    }
}

int main() {
    test(1, 2);
    test(2, 1);
    test(1, 1);
    return 0;
}