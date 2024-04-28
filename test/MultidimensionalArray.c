/* #include <stdio.h> */

int printArr(int arr[3][3], int m, int n) {
    int i, j;
    for (i = 0;i < m;i++) {
        for (j = 0;j < n;j++) {
            printf("%d ", arr[i][j]);
        }
        puts("");
    }
    return 0;
}

int transpose(int arr[3][3], int m, int n) {
    int i, j, temp;
    for (i = 0;i < m;i++) {
        for (j = 0;j < n;j++) {
            if (i < j) {
                temp = arr[i][j];
                arr[i][j] = arr[j][i];
                arr[j][i] = temp;
            }
        }
    }
}

int main() {
    int arr[3][3] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    printf("before transpose\n");
    printArr(arr, 3, 3);
    transpose(arr, 3, 3);
    printf("after transpose\n");
    printArr(arr, 3, 3);
    return 0;
}