#include <stdlib.h>
#include "include/mcc.h"
// #include "include/util.h"

char *new_str()
{
    return (char *)malloc(0);
}

char *str_push(char *str, char c)
{
    // printf("0X%p\n", str);
    int new_len = strlen(str) + 1;
    char *new_str = (char *)realloc(str, new_len);
    char *rs = strncat(new_str, &c, 1);
    // printf("0X%p\n", new_str);
    return rs;
}

char *str_pop(char *str)
{
    int new_len = strlen(str) - 1;
    *(str + new_len) = '\0';
    return str;
}

static Token *new_token(int id, int line, TOKEN_TYPE type, char *value)
{
    Token *token = (Token *)malloc(sizeof(Token));
    token->id = id;
    token->line = line;
    token->type = type;
    token->value = value;
    token->next = NULL;
    return token;
}

Token *Lexer(char *str)
{
    static Token head, *p = &head;
    int i = 0;
    int line = 1;
    int count = 0;
    while (str[i] != EOF)
    {
        if (str[i] == ' ' || str[i] == '\n' || str[i] == '\t')
        {
            if (str[i] == '\n')
            {
                line++;
            }
            i++;
            continue;
        }
        /* 标识符/关键字 */
        if (isAlpha(str[i]) || str[i] == '_')
        {
            char *s = new_str();
            do
            {
                str_push(s, str[i]);
                i++;
            } while (isAlpha(str[i]) || isDigit(str[i]) || str[i] == '_');
            Token *token = new_token(count, line, (isKeyWord(s) ? KEYWORD : IDENTIFIER), s);
            p->next = token;
            p = p->next;
            count++;
            continue;
        }
        /* 整型数字常量 */
        if (isDigit(str[i]))
        {
            char *s = new_str();
            do
            {
                str_push(s, str[i]);
                i++;
            } while (isDigit(str[i]));
            Token *token = new_token(count, line, NUMBER, s);
            p->next = token;
            p = p->next;
            count++;
            continue;
        }
        /* 字符串常量 */
        if (str[i] == '\"')
        {
            char *s = new_str();
            str_push(s, str[i++]);
            while (str[i] != '\"')
            {
                if (str[i] == '\\' && str[i + 1] && str[i + 1] == '\"')
                {
                    str_push(s, str[i]);
                    i++;
                    str_push(s, str[i]);
                    i++;
                    continue;
                }
                str_push(s, str[i]);
                i++;
            }
            str_push(s, str[i]);
            i++;
            Token *token = new_token(count, line, STRING, s);
            p->next = token;
            p = p->next;
            count++;
            continue;
        }
        /* 注释 */
        if (str[i] == '/' && str[i + 1] && str[i + 1] == '*')
        {
            char *s = new_str();
            str_push(s, str[i]);
            i++;
            str_push(s, str[i]);
            i++;
            while (!(str[i] == '*' && str[i + 1] && str[i + 1] == '/'))
            {
                str_push(s, str[i]);
                i++;
            }
            str_push(s, str[i]);
            i++;
            str_push(s, str[i]);
            i++;
            // Token *token = new_token(count, line, COMMENT, s);
            // p->next = token;
            // p = p->next;
            // count++;
            continue;
        }
        /* 界符 */
        char *s = (char *)malloc(0);
        s = str_push(s, str[i]);
        if (isBoundarySign(s))
        {
            do
            {
                i++;
                s = str_push(s, str[i]);
            } while (isBoundarySign(s));
            str_pop(s);
            Token *token = new_token(count, line, BOUNDARYSIGN, s);
            p->next = token;
            p = p->next;
            count++;
            continue;
        }
        /* 运算符 */
        if (isOperator(s))
        {
            do
            {
                i++;
                s = str_push(s, str[i]);
            } while (isOperator(s));
            str_pop(s);
            Token *token = new_token(count, line, OPERATOR, s);
            p->next = token;
            p = p->next;
            count++;
            continue;
        }
    }
    Token *token = new_token(count, line + 1, TK_EOF, NULL);
    p->next = token;
    p = p->next;
    count++;
    return head.next;
}