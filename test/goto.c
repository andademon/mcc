/* #include <stdio.h> */

int main() {
    Label1: {
        printf("in Label1\n");
        goto Label3;
    }
    Label2: {
        printf("in Label2\n");
    }
    Label3: {
        printf("in Label3\n");
    }
    return 0;
}