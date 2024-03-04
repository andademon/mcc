#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "include/mcc.h"
// #include "include/util.h"

static Token *head;

void printToken(Token *token) {
    printf("%d\t%d\t%d\t%s\n", token->id, token->line, token->type, token->value);
}

void printTokenList(Token *tokens) {
    printf("ID\tLINE\tTYPE\tVALUE\n");
    Token *current_token = tokens;
    while (current_token)
    {
        printToken(current_token);
        current_token = current_token->next;
    }
}

void printTab(int num) {
    if (num < 0) return;
    for(int i = 0;i < num;i++) {
        printf("    ");
    }
}

/**
 * TODO: 深度优先遍历
 * print type_name
 * 如果body是list -> for 每个 body list 节点 深度优先遍历
 * 如果不是list -> 深度优先遍历 body
 * 
 * 特殊: blockStmt不是list
 * 但是body为空，它的body被拆分为decl和stmt list
 * 注: 已修改
 * 
 * expr和stmt的拆分问题
 * 
 * 更新：直接按node_type 分类处理各种节点
 * 
*/

void printNode(Node *node, int tabs) {
    if (node == NULL) return;
    printTab(tabs);
    printf("%s\n", node->type_name); // 公共属性 type_name
    Node *p = NULL;
    switch(node->node_type) {
        case ND_ASSIGN_EXPR:
        case ND_BINARY_EXPR:
            printTab(tabs + 1);
            printf("left: \n");
            printNode(node->lhs, tabs + 2);
            printTab(tabs + 1);
            printf("right: \n");
            printNode(node->rhs, tabs + 2);
            printTab(tabs + 1);
            printf("op: %s\n", node->op->value);
            break;
        case ND_BLOCK:
            p = node->body;
            while(p != NULL) {
                printNode(p, tabs+1);
                p = p->next;
            }
            break;
        case ND_BREAK:
            break;
        case ND_DECL_LIST:
            p = node->body;
            while(p != NULL) {
                printNode(p, tabs+1);
                p = p->next;
            }
            break;
        case ND_EXPR_STMT:
            printNode(node->body, tabs + 1);
            break;
        case ND_FOR:
            break;
        case ND_FUNCALL:
            break;
        case ND_FUNC_DECL:
            printTab(tabs + 1);
            printf("id: %s\n", node->id->value);
            printTab(tabs + 1);
            printf("params: \n");
            Node *params = node->params;
            while (params != NULL)
            {
                printNode(params, tabs + 2);
                params = params->next;
            }
            printTab(tabs + 1);
            printf("body: \n");
            printNode(node->body, tabs + 2);
            break;
        case ND_FUNC_PARAM:
            printTab(tabs + 1);
            printf("id: %s\n", node->id->value);
            break;
        case ND_IDENT:
            printTab(tabs + 1);
            printf("id: %s\n", node->id->value);
            break;
        case ND_IF_STMT:
            printTab(tabs + 1);
            printf("test: \n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("consequent: \n");
            printNode(node->consequent, tabs + 2);
            printTab(tabs + 1);
            printf("alternative: \n");
            printNode(node->alternative, tabs + 2);
            break;
        case ND_NUM:
            printTab(tabs + 1);
            printf("value: %s\n", node->tok->value);
            break;
        case ND_RETURN:
            printNode(node->body, tabs + 1);
            break;
        case ND_SWITCH:
            break;
        case ND_STMT_LIST:
            p = node->body;
            while(p != NULL) {
                printNode(p, tabs+1);
                p = p->next;
            }
            break;
        case ND_VAR_DECL:
            printTab(tabs + 1);
            printf("id: %s\n", node->id->value);
            printNode(node->body, tabs + 1);
            break;
        case ND_WHILE:
            break;
        default:
            printNode(node->body, tabs + 1);
            break;
    }
    // if (node->id != NULL) {
    //     printTab(tabs + 1);
    //     printf("id: %s\n", node->id->value);
    // }
    // if (node->node_type == ND_FUNC_DECL) {
    //     printTab(tabs + 1);
    //     printf("id: %s\n", node->id->value);
    //     printTab(tabs + 1);
    //     printf("params: \n");
    //     Node *params = node->params;
    //     while (params != NULL)
    //     {
    //         printNode(params, tabs + 2);
    //         params = params->next;
    //     }
    //     printTab(tabs + 1);
    //     printf("body: \n");
    //     printNode(node->body, tabs + 2);
    //     return;
    // }
    // if (node->node_type == ND_BINARY_EXPR) {
    //     printNode(node->lhs, tabs + 1);
    //     printNode(node->rhs, tabs + 1);
    //     printTab(tabs + 1);
    //     printf("op: %s\n", node->op->value);
    //     return;
    // }
    // if (node->is_list) {
    //     Node *p = node->body;
    //     while(p != NULL) {
    //         printNode(p, tabs+1);
    //         p = p->next;
    //     }
    // }
    // else {
    //     printNode(node->body, tabs + 1);
    // }
}

void printProgram(Program *prog) {
    printf("Program\n");
    printf("    body: \n");
    printNode(prog->body, 2);
}

int main(int argc, char *argv[])
{
    // char *str = readFile("demo.c");
    char *str = readFile("D:\\git_workplace\\mcc\\demo.c");
    // printf("%s\n", str);
    head = Lexer(str);

    printTokenList(head);
    Program *prog = parse(head);
    printProgram(prog);
    return 0;
}