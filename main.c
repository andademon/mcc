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
        printf("   ");
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
*/

void printNode(Node *node, int tabs) {
    if (node == NULL) return;
    printTab(tabs);
    printf("%s\n", node->type_name);
    if (node->id != NULL) {
        printTab(tabs + 1);
        printf("id: %s\n", node->id->value);
    }
    if (node->type == ND_FUNC_DECL) {
        // printTab(tabs + 1);
        // printf("id: %s\n", node->id->value);
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
        return;
    }
    if (node->type == ND_BINARY_EXPR) {
        printNode(node->lhs, tabs + 1);
        printNode(node->rhs, tabs + 1);
        printTab(tabs + 1);
        printf("op: %s\n", node->op->value);
        return;
    }
    if (node->is_list) {
        Node *p = node->body;
        while(p != NULL) {
            printNode(p, tabs+1);
            p = p->next;
        }
    }
    else {
        printNode(node->body, tabs + 1);
    }
}
void printProgram(Program *prog) {
    printf("Program\n");
    // printf("type: \"program\"\n");
    printf("Body\n");
    printNode(prog->body, 0);
    // if (prog->body != NULL) {
    //     Node *p = prog->body;
    //     while (p != NULL)
    //     {
    //         // printfDecl(p);
    //         printf("%s\n", p->type_name);
    //         if (p->is_list == true) {
    //             // p = p->list;
    //             p = p->body;
    //         }
    //         p = p->next;
    //     }
    // }
}

int main(int argc, char *argv[])
{
    char *str = readFile("demo.c");
    printf("%s\n", str);
    head = Lexer(str);

    printTokenList(head);
    Program *prog = parse(head);
    printProgram(prog);
    return 0;
}
