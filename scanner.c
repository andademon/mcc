#include "include/mcc.h"

char *new_str()
{
    char *str = (char *)malloc(1);
    if (str != NULL) {
        memset(str, 0, 1);
    }
    return str;
}

char *str_push(char *str, char c)
{
    int new_len = strlen(str) + 1;
    char *new_str = (char *)realloc(str, new_len + 1); // last byte store '\0'
    if (new_str != NULL) {
        new_str[new_len - 1] = c;
        new_str[new_len] = '\0';
    }
    return new_str;
}

char *str_pop(char *str)
{
    int new_len = strlen(str) - 1;
    *(str + new_len) = '\0';
    return str;
}

static Token *new_token(int id, int line, int token_type, char *value) {
    Token *token = (Token *)malloc(sizeof(Token));
    token->id = id;
    token->line = line;
    token->token_type = token_type;
    token->value = value;
    token->next = NULL;
    return token;
}

Token *Lexer(char *str)
{
    int len = strlen(str);
    static Token head, *p = &head;
    int i = 0;
    int line = 1;
    int count = 0;
    while (str[i] != '\0' && i <= len)
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
        /* 字符常量 */
        if (str[i] == '\'')
        {
            i++;
            char *s = new_str();
            if (str[i] == '\'')
            {
                str_push(s, str[i]);
                i++;
            }
            str_push(s, str[i]);
            i += 2;
            Token *token = new_token(count, line, CHARACTER, s);
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
        char *s = new_str();
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