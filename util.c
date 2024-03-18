#include "include/mcc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define KEYWORD_LEN 31
#define OPERATOR_LEN 23
#define BOUNDARYSIGN_LEN 11

/* C89关键字 */
static char* kw[] = {
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "int",
    "long",
    "register",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "unsigned",
    "union",
    "void",
    "volatile"
    "while",
};

/* 运算符 */
static char* op[] = {
    "+",
    "-",
    "*",
    "/",
    "%",
    "++",
    "--",
    "=",
    ">",
    "<",
    ">=",
    "<=",
    "==",
    "!=",
    "!",
    "&&",
    "||",
    ".",
    "?",
    ":",
    "&",
    "|",
    "\\",
};

/* 界符 */
static char* bound[] = {
    "{",
    "}",
    "(",
    ")",
    "[",
    "]",
    ",",
    ";",
    "#",
    "\'",
    "\"",
};

/* 是否是关键字 */
int isKeyWord(char *str)
{
    int i = 0;
    while (i < KEYWORD_LEN)
    {
        if (strcmp(str, kw[i]) == 0)
        {
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

/* 是否是运算符 */
int isOperator(char *str)
{
    int i = 0;
    while (i < OPERATOR_LEN)
    {
        if (strcmp(str, op[i]) == 0)
        {
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

/* 是否是界符 */
int isBoundarySign(char *str)
{
    int i = 0;
    while (i < BOUNDARYSIGN_LEN)
    {
        if (strcmp(str, bound[i]) == 0)
        {
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

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
    *(buffer + fileSize) = '\0';
    return buffer;
}

File *new_file(char *filename, char *path) {
    File *file = (File *)malloc(sizeof(File));
    file->filename = filename;
    file->path = path;
    return file;
}
