
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    else {
        return fibonacci(n-1) + fibonacci(n-2);
    }
}

int main() {
    int n = 10, i;
    printf("斐波那契数列的前 %d 项为：\n", n);
    for (i = 0; i < n; i = i + 1) {
        printf("%d\n", fibonacci(i));
    }
    return 0;
}