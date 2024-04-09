void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int main() {
    int a = 3, b = 5;
    swap(&a, &b);
    printf("a: %d, b: %d\n", a, b);
    return 0;
}