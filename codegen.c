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

// 全局变量保存在静态数据段
static void gen_gvar(Node *node) {
    if (node->node_type != ND_VAR_DECL) return;

    int len = node->declarators->len;

    for (int i = 0;i < node->declarators->len;i++) {
        Node *temp_node = node->declarators->data[i];
        printf("%s:\n", temp_node->id->value);
        if (temp_node->init == NULL) {
            int size = get_size(node->decl_type);
            printf("\t.zero\t%d\n", size);
            return;
        }
        else {
            switch (temp_node->decl_type) {
                case INT: {
                    printf("\t.word\t%s\n", temp_node->init->tok->value);
                    break;
                }
                case CHAR: {
                    printf("\t.byte\t%d\n", (int)temp_node->init->tok->value[0]);
                    break;
                }
            }
        }
    }
}

static int offset = 0;

// 将局部变量保存在内存中，返回记录的偏移地址（Q: 偏移地址如何计算？）
static void gen_lvar(Node *node) {

    if (node->node_type != ND_VAR_DECL) return;

    // 无初始化，跳过
    // 有初始化，且初始化类型为以下之一：
    // INT: NUM -> 加载到寄存器，然后保存到内存，在符号表中记录在内存中的偏移量 -> li，sw
    // CHAR: CHARACTER, li, sb
    // BOOL: TRUTH, 同上 li, sb
    int len = node->declarators->len;

    for (int i = 0;i < len;i++) {
        Node *temp_node = node->declarators->data[i];
        // 当且仅当变量有赋初值且为常量赋值时
        if (temp_node->init == NULL 
            || (temp_node->init->node_type != ND_NUM 
                && temp_node->init->node_type != ND_CHAR)
        )
            continue;
        // int imm = (int)temp_node->init->value
        int reg = new_reg();
        switch(node->decl_type) {
            case INT: {
                printf("li r%d, %s\n", reg, temp_node->init->tok->value);
                printf("sw r%d, -%d(s0)\n", reg, offset);
                break;
            }
            case CHAR: {
                printf("li r%d, %d\n", reg, (int)temp_node->init->tok->value[0]);
                printf("sb r%d, -%d(s0)\n", reg, offset);
                break;
            }
        }
        offset += 4;
    }
}

static int gen_stmt(Node *node){

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
    // gen_gvar(node);
    gen_lvar(node);
}

void codegen_test(Node *node) {
    gen_gvar(node);
}