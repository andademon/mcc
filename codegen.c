#include <stdio.h>
#include <stdlib.h>
#include "include/mcc.h"

static void emit(char *op, int r0, int r1, int r2);

void emit_data(Node *node);
void emit_code(Node *node);
static BB *new_bb();
static IR *new_ir();
static int new_reg();
static int gen_stmt(Node *node);
static int gen_binop(Node *node);
static int gen_expr(Node *node);

static int reg = 0;

static int new_reg() {
    return reg++;
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
    }
}

static void emit(char *op, int r0, int r1, int r2) {
    printf("%d = %d %s %d\n", r0, r1, op, r2);
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
    gen_expr(node);
}