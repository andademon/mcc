/* #include <stdio.h> */

int printArr(int arr[3][3], int x, int y) {
    printf("%d\n", arr[x][y]);
    return 0;
}


int main() {
    int arr[3][3];
    arr[0][0] = 1;
    arr[1][1] = 2;
    arr[2][2] = 3;
    printArr(arr, 0, 0);
    printArr(arr, 1, 1);
    printArr(arr, 2, 2);
    return 0;
}