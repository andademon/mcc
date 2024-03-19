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

Vector *new_vec() {
    Vector *v = malloc(sizeof(Vector));
    v->data = malloc(sizeof(void *) * 16);
    v->capacity = 16;
    v->len = 0;
    return v;
}

void vec_push(Vector *v, void *elem) {
    if (v->len == v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, sizeof(void *) * v->capacity);
    }
    v->data[v->len++] = elem;
}

void vec_pushi(Vector *v, int val) {
  vec_push(v, (void *)(intptr_t)val);
}

void *vec_pop(Vector *v) {
    return v->data[--v->len];
}

void *vec_last(Vector *v) {
    return v->data[v->len - 1];
}

bool vec_contains(Vector *v, void *elem) {
    for(int i = 0;i < v->len;i++)
        if (v->data[i] == elem) 
            return true;
    return false;
}

Map *new_map() {
    Map *map = (Map *)malloc(sizeof(Map));
    map->keys = new_vec();
    map->vals = new_vec();
    return map;
}

void map_put(Map *map, char *key, void *val) {
    vec_push(map->keys, key);
    vec_push(map->vals, val);
}

void map_puti(Map *map, char *key, int val) {
  map_put(map, key, (void *)(intptr_t)val);
}

void *map_get(Map *map, char *key) {
    for (int i = map->keys->len - 1;i >= 0;i--)
        if (!strcmp(map->keys->data[i], key))
            return map->vals->data[i];
    return NULL;
}

int map_geti(Map *map, char *key, int default_) {
  for (int i = map->keys->len - 1; i >= 0; i--)
    if (!strcmp(map->keys->data[i], key))
      return (intptr_t)map->vals->data[i];
  return default_;
}
