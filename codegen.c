#include <stdio.h>
#include <stdlib.h>
#include "include/mcc.h"

void emit_data(Var *var);
void emit_code(Function *fn);
static void emit(char *op, Reg *r0, Reg *r1, Reg *r2);
static void emit2(char *op, int r0, int r2);

static Reg *new_reg();
static BB *new_bb();
static IR *new_ir();
static void gen_stmt(Node *node);
static Reg *gen_binop(Node *node);
static Reg *gen_expr(Node *node);
// 全局变量放静态数据段中，局部变量则先将数据加载到寄存器中，然后用内存保存
static void gen_gvar(Var *var);
static void gen_lvar(Var *var);

int nlabel = 1;
int nreg = 1;

static Vector *irs;
static BB *out;

static Map *SymbolTable;

static Reg *new_reg() {
    Reg *r = calloc(1, sizeof(Reg));
    r->vn = nreg++;
    r->rn = -1;
    return r;
}

static IR *new_ir() {
    return (IR *)calloc(1, sizeof(IR));
}

static BB *new_bb() {
    BB *bb = (BB *)calloc(1, sizeof(BB));
    bb->label = nlabel++;

    bb->ir = new_vec();
    bb->in_regs = new_vec();
    bb->out_regs = new_vec();
    return bb;
}

static int get_size(int type) {
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

// 全局变量保存在静态数据段
static void gen_gvar(Var *var) {
    printf("%s:\n", var->name);
    if (var->init == NULL) {
        int size = get_size(var->type);
        printf("\t.zero\t%d\n", size);
        return;
    }
    else {
        switch (var->type) {
            case INT: {
                printf("\t.word\t%s\n", var->init->tok->value);
                break;
            }
            case CHAR: {
                printf("\t.byte\t%d\n", (int)var->init->tok->value[0]);
                break;
            }
        }
    }
}

static int offset = 0;

// 将局部变量保存在内存中，返回记录的偏移地址（Q: 偏移地址如何计算？）
static void gen_lvar(Var *var) {
    // 无初始化，跳过
    // 有初始化，且初始化类型为以下之一：
    // INT: NUM -> 加载到寄存器，然后保存到内存，在符号表中记录在内存中的偏移量 -> li，sw
    // CHAR: CHARACTER, li, sb
    // BOOL: TRUTH, 同上 li, sb

    // 当且仅当变量有赋初值且为常量赋值时
    if (var->init == NULL 
        || (var->init->node_type != ND_NUM 
            && var->init->node_type != ND_CHAR)
    )
        return;

    Reg *reg = new_reg();
    var->offset = offset;
    switch(var->type) {
        case INT: {
            printf("li r%d, %s\n", reg->vn, var->init->tok->value);
            printf("sw r%d, -%d(s0)\n", reg->vn, offset);
            break;
        }
        case CHAR: {
            printf("li r%d, %d\n", reg->vn, (int)var->init->tok->value[0]);
            printf("sb r%d, -%d(s0)\n", reg->vn, offset);
            break;
        }
    }
    offset += 8;
}

// 生成二元运算表达式IR
static Reg *gen_binop(Node *node) {
    Reg *r0 = new_reg();
    Reg *r1 = gen_expr(node->lhs);
    Reg *r2 = gen_expr(node->rhs);
    emit(node->op->value, r0, r1, r2);
    return r0;
}

// 生成expr IR
static Reg *gen_expr(Node *node) {
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
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->rhs);
            break;
        }
        case ND_FUNCALL: {
            Reg *args[6];
            // 对于函数参数的处理，如果是立即数，直接li加载，否则（是左值）看arg在哪里保存
            for (int i = 0;i < node->args->len;i++)
                args[i] = gen_expr(node->args->data[i]);
            Reg *r0 = new_reg();
            printf("call %s\n", node->tok->value);
            printf("mv r%d, a0\n", r0->vn);
            return r0;
            break;
        }
    }
}

static void emit(char *op, Reg *r0, Reg *r1, Reg *r2) {
    printf("%s r%d, r%d, r%d\n", op, r0->vn, r1->vn, r2->vn);
}

// 右值（值）
static void gen_rval(Node *node) {

}

// 对于每一个左值，符号表中记录该变量在内存中的偏移量

// 左值（地址） 有如下详细解释：
// In C, all expressions that can be written on the left-hand side of
// the '=' operator must have an address in memory. In other words, if
// you can apply the '&' operator to take an address of some
// expression E, you can assign E to a new value.
//
// Other expressions, such as `1+2`, cannot be written on the lhs of
// '=', since they are just temporary values that don't have an address.
//
// The stuff that can be written on the lhs of '=' is called lvalue.
// Other values are called rvalue. An lvalue is essentially an address.
//
// When lvalues appear on the rvalue context, they are converted to
// rvalues by loading their values from their addresses. You can think
// '&' as an operator that suppresses such automatic lvalue-to-rvalue
// conversion.
//
// This function evaluates a given node as an lvalue.
static void gen_lval(Node *node) {

}

static void gen_stmt(Node *node) {
    switch (node->node_type) {
        case ND_BLOCK: {
            BB *body = new_bb();
            for (int i = 0;i < node->stmts->len;i++) {
                gen_stmt(node->stmts->data[i]);
            }
        }
        case ND_WHILE_STMT: {
            BB *test = new_bb();
            BB *body = new_bb(); // 
            BB *break_ = new_bb(); // while_stmt break后的去处
        }
        case ND_FOR_STMT: {
            BB *init = new_bb();
            BB *test = new_bb();
            BB *update = new_bb();
            BB *body = new_bb();
        }
        case ND_IF_STMT: {
            BB *then = new_bb();
            BB *els = new_bb();
            BB *last = new_bb();
        }
        case ND_EXPR_STMT: {
            gen_expr(node->body);
            break;
        }
    }
}

static void gen_param(Node *node, int i) {
    Reg *r0 = new_reg();
    printf("mv r%d,a%d\n", r0->vn, i);
    printf("sw r%d,-%d(s0)\n", r0->vn, offset);
    offset += 8;
}

int compute_var_size(Var *var) {
    int size = get_size(var->type);
    size = (size < 4) ? 4 :size;
    if (var->is_array) return size * var->len;
    return size;
}

int compute_function_stack_size(Function *func) {    
    int count_size = 0;
    for (int i = 0;i < func->lvars->len;i++) {
        Var *var = func->lvars->data[i];
        count_size += compute_var_size(var);
    }
    return count_size + 32;
}

void emit_data(Var *var) {
    gen_gvar(var);
}

void emit_code(Function *fn) {    
    // 函数代码生成前计算函数栈需要多少空间
    int stack_size = compute_function_stack_size(fn);

    offset = 0;

    printf(".%s\n", fn->name);

    printf("addi    sp,sp,-%d\n", stack_size);
    printf("sd      ra,24(sp)\n");
    printf("sd      s0,16(sp)\n");
    printf("addi    s0,sp,%d\n", stack_size);

    puts("");

    for (int i = 0;i < fn->node->params->len;i++) {
        gen_param(fn->node->params->data[i], i);
    }
    for (int i = 0;i < fn->lvars->len;i++) {
        gen_lvar(fn->lvars->data[i]);
    }
    // for (int i = 0;i < node->body->stmts->len;i++) {
    //     gen_stmt(node->body->stmts->data[i]);
    // }

    puts("");

    printf("ld      ra,24(sp)\n");
    printf("ld      s0,16(sp)\n");
    printf("addi    sp,sp,%d\n", stack_size);
    printf("jr      ra\n");
}

void codegen(Program *prog) {
    if (!irs) irs = new_vec();
    for (int i = 0;i < prog->gvars->len;i++) {
        emit_data(prog->gvars->data[i]);
        puts("");
    }
    for (int i = 0;i < prog->funcs->len;i++) {
        emit_code(prog->funcs->data[i]);
        puts("");
    }
}

int compute_offset(Var *var) {

}