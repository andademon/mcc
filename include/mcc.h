#ifndef __MCC_H__
#define __MCC_H__
#include <string.h>
#define TRUE 1
#define FALSE 0

/* (Alpha | _)(Alpha | Digit | _)* */
/* (Digit)(Digit)* */
/* "\"(* except for \")\"" */
typedef enum
{
    KEYWORD, /* 关键字 */
    IDENTIFIER, /* 标识符 */
    NUMBER, /* 数字 */
    STRING, /* 字符串 */
    OPERATOR, /* 运算符 */
    BOUNDARYSIGN, /* 界符 */
    COMMENT, /* 注释 */
    OTHER, /* 其他 */
} TOKEN_TYPE;

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
    while (kw[i])
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
    while (op[i])
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
    while (bound[i])
    {
        if (strcmp(str, bound[i]) == 0)
        {
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

#endif