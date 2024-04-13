void printArray(int arr[], int size) {
    int i;
    for (i = 0; i < size; i++)
        printf("%d ", arr[i]);
    printf("\n");
}

int main() {
    int a[6] = {10, 5, 7, 1, 3, 2};
    printArray(a, 6);
    return 0;
}