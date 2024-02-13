#include <stdio.h>
#include <string.h>

/* main function */
int main()
{
    int a = 1, b = 2;
    printf("a+b=%d\n", a + b);
    printf("\"\"\"111");
    char *str1 = '#';
    char *str2 = "#";
    printf("%d", strcmp(str1, str2));
    return 0;
}