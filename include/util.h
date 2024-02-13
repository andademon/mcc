#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdio.h>
#include <stdlib.h>

/* 是否是字母 */
int isAlpha(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
    {
        return 1;
    }
    return 0;
}

/* 是否是数字 */
int isDigit(char c)
{
    if (c >= '0' && c <= '9')
    {
        return 1;
    }
    return 0;
}

/* 返回文件大小 */
long getFileSize(char *filename)
{
    FILE *fp;
    if ((fp = fopen(filename, "r")) == NULL)
    {
        printf("Error! Can't open file: %s", filename);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    return fileSize;
}

/* 读取文件，返回指向字符串的指针 */
char *readFile(char *filename)
{
    FILE* fp;
    if ((fp = fopen(filename, "r")) == NULL)
    {
        printf("Error! Can't open file: %s", filename);
        return NULL;
    }
    /* 获取文件大小 */
    long fileSize = getFileSize(filename);
    /* 读取文件 */
    char *buffer = (char *) malloc((fileSize + 1) * sizeof(char));
    fread(buffer, sizeof(char), fileSize, fp);
    fclose(fp);
    *(buffer + fileSize) = EOF;
    return buffer;
}

#endif