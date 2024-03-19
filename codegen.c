#include <stdio.h>
#include <stdlib.h>
#include "include/mcc.h"

void emit_data(Node *node);
void emit_code(Node *node);
static void emit(char *op, int r0, int r1, int r2);
static void emit2(char *op, int r0, int r2);

static BB *new_bb();
static IR *new_ir();
static int new_reg();
static int gen_stmt(Node *node);
static int gen_binop(Node *node);
static int gen_expr(Node *node);
// 全局变量放静态数据段中，局部变量则先将数据加载到寄存器中，然后用内存保存
static void gen_gvar(Node *node);
static void gen_lvar(Node *node);

static int get_size(TYPE type) {
    switch (type) {
        case CHAR:
            return 1;
        case INT:
            return 4;
        default:
            return 0;
    }
}

static char *get_size_name(int size) {
    switch (size) {
        case 1:
            return ".byte";
        case 4:
            return ".word";
        default:
            return NULL;
    }
}

static int reg = 0;

static int new_reg() {
    return reg++;
}

static void gen_gvar(Node *node) {
    if (node->node_type != ND_VAR_DECLARATOR) return;
    printf("%s:\n", node->id->value);
    int size = get_size(node->decl_type);
    if (node->init == NULL) {
        printf("\t.zero\t%d", size);
        return;
    }
    else {
        // CHARACTER
        if (node->init->tok->type == CHARACTER) {
            printf("\t%s\t%d", get_size_name(size), (int)node->init->tok->value[0]);
        }
        // NUMBER
        else {
            printf("\t%s\t%s", get_size_name(size), node->init->tok->value);
        }
        return;
    }
}

static void gen_lvar(Node *node) {
    
}

static int gen_binop(Node *node) {
    int r0 = new_reg();
    int r1 = gen_expr(node->lhs);
    int r2 = gen_expr(node->rhs);
    emit(node->op->value, r0, r1, r2);
    return r0;
}

static int gen_expr(Node *node) {
    switch (node->node_type) {
        case ND_BINARY_EXPR:
            return gen_binop(node);
            break;
        case ND_NUM:
            return new_reg();
            break;
        case ND_IDENT:
            return new_reg();
            break;
        case ND_ASSIGN_EXPR: {
            int r0 = new_reg();
            int r1 = gen_expr(node->rhs);
            break;
        }
    }
}

static void emit(char *op, int r0, int r1, int r2) {
    printf("%s r%d, r%d, r%d\n", op, r0, r1, r2);
}

static BB *new_bb() {
    return (BB *)calloc(1, sizeof(BB));
}

static IR *new_ir() {
    return (IR *)calloc(1, sizeof(IR));
}

// void emit_data(Node *node) {

// }

// static BB *bb = NULL;

void emit_code(Node *node) {
    switch(node->node_type) {

    }
}

void codegen(Node *node) {
    // gen_expr(node);
    gen_gvar(node);
}