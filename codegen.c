#include <stdio.h>
#include <stdlib.h>
#include "include/mcc.h"

void emit_data(Var *var);
void emit_code(Function *fn);
static void emit(int op, Reg *r0, Reg *r1, Reg *r2);

static Reg *new_reg();
static BB *new_bb();
static IR *new_ir();
static void gen_stmt(Node *node);
static Reg *gen_binop(Node *node);
static Reg *gen_expr(Node *node);
// 全局变量放静态数据段中，局部变量则先将数据加载到寄存器中，然后用内存保存
static void gen_gvar(Var *var);
static void gen_lvar(Var *var);

static Reg *gen_lval(Node *node);
static char *get_lval_access_unit(Node *node);
static IR *br(Reg *r, BB *then, BB *els);
static IR *jmp(BB *bb);

static Reg *load();
static void store();

int nlabel = 1; // label count
int nreg = 0; // real register count
int clabel = 0; // constant label count
// int offset = 0;

static SymbolTable *table; // 符号表
static SymbolTable *currentScope; // 当前作用域符号表 
static Function *currentFunction; // 当前函数
static BB *currentBB; // 指向当前basic block
static BB *returnBB; // 保存函数返回时的basic block, 于函数初始化时同时初始化
static BB *break_; // 临时basic block, 用于指向break语句的返回block, codegen初始化为NULL
static BB *continue_; // 临时basic block, 用于指向continue语句的返回block, codegen初始化为NULL

static Vector *registers;

static void nop() {
    printf("nop\n\n");
}

// 分支跳转IR,绑定真分支与跳转分支
static IR *br(Reg *r, BB *then, BB *els) {
    printf("beq t%d,zero,.L%d\n", r->vn, els->label);
    jmp(then);
    IR *ir = new_ir();
    ir->r2 = r;
    ir->bb1 = then;
    ir->bb2 = els;
    return ir;
}

// 无条件跳转IR,跳转到指定基本块
static IR *jmp(BB *bb) {
    printf("j .L%d\n", bb->label);
    IR *ir = new_ir();
    ir->op = IR_J;
    ir->bb1 = bb;
    return ir;
}

static int new_label() {
    return nlabel++;
}

static int new_constant_label() {
    return clabel++;
}

static Reg *new_real_reg() {
    Reg *r = calloc(1, sizeof(Reg));
    r->vn = nreg++;
    r->rn = -1;
    r->using = false;
    return r;
}

static Reg *new_reg() {
    for (int i = 0;i < registers->len;i++) {
        Reg *r = registers->data[i];
        if (r->using == false) {
            r->using = true;
            return r;
        }
    }
    printf("register allocate error\n");
    exit(1);
}

static void kill_reg(Reg *reg) {
    reg->using = false;
}

static IR *new_ir() {
    IR *ir = (IR *)calloc(1, sizeof(IR));
    if (currentBB) vec_push(currentBB->ir, ir);
    return ir;
}

static BB *new_bb() {
    BB *bb = (BB *)calloc(1, sizeof(BB));
    bb->label = nlabel++;

    bb->ir = new_vec();
    bb->in_regs = new_vec();
    bb->out_regs = new_vec();
    if (currentFunction) vec_push(currentFunction->bbs, bb);
    return bb;
}

// 全局变量保存在静态数据段
static void gen_gvar(Var *var) {
    printf("%s:\n", var->name);
    if (var->init == NULL) {
        printf("\t.zero\t%d\n", var->type->size);
        return;
    }
    else {
        switch (var->type->kind) {
            case TY_INT: {
                printf("\t.word\t%s\n", var->init->token->value);
                break;
            }
            case TY_CHAR: {
                printf("\t.byte\t%d\n", (int)var->init->token->value[0]);
                break;
            }
            // TODO: pointer/array初始化
            case TY_POINTER_TO: {
                printf("\t.dword\t%s\n", var->init->token->value);
                break;
            }
            case TY_ARRAY_OF: {
                // 数组元素赋值,基类是直到不是数组的下一个类型
                Type *ty = var->type;
                while(ty->kind == TY_ARRAY_OF) {
                    ty = ty->base;
                }
                
                for (int i = 0;i < var->init->args->len;i++) {
                    Node *arg = var->init->args->data[i];
                    if (arg->node_type == ND_NUM) {
                        printf("\t.word\t%s\n", arg->token->value);
                    }
                    else if (arg->node_type == ND_CHAR) {
                        printf("\t.byte\t%d\n", (int)arg->token->value[0]);
                    }
                    else {
                        printf("\t%s\t%s\n", get_access_unit(ty->align), arg->token->value);
                    }
                }
                break;
            }
        }
    }
}

// 将局部变量保存在内存中，返回记录的偏移地址（Q: 偏移地址如何计算？）
static void gen_lvar(Var *var) {
    // 1.移动offset,在stack中预留变量空间
    // 2.记录变量offset
    // 3.如有初始化则初始化

    // offset -= var->type->size;
    // var->offset = offset;

    if (var->init == NULL) return;
    // TODO: 多维数组初始化
    // 目前只支持按顺序逐个初始化
    // 参考https://learn.microsoft.com/en-ie/cpp/c-language/initializing-aggregate-types
    if (var->type->kind == TY_ARRAY_OF && var->init->node_type == ND_ARR_EXPR) {
        Type *ty = var->type;
        while (ty->kind == TY_ARRAY_OF) {
            ty = ty->base;
        }
        for (int i = 0;i < var->init->args->len;i++) {
            Node *arg = var->init->args->data[i];
            Reg *r0 = gen_expr(arg);
            printf("s%s t%d,%d(s0)\n", get_access_unit(ty->align), r0->vn, (var->offset + (ty->align * i)));
            kill_reg(r0);
        }
    }
    // if (var->type->kind == TY_ARRAY_OF) {
    //     Vector *args = var->init->args;
    //     for (int i = 0;i < args->len;i++) {
    //         Reg *r0 = gen_expr(args->data[i]);
    //         printf("s%s t%d,%d(s0)\n", get_access_unit(var->type_size), r0->vn, offset + var->type_size * i);
    //         kill_reg(r0);
    //     }
    //     return;
    // }
    else {
        type_expr(var->init, currentScope);
        Reg *r0 = gen_expr(var->init);
        printf("s%s t%d,%d(s0)\n", get_access_unit(var->type->align), r0->vn, var->offset);
        kill_reg(r0);
    }
}

// 生成一元运算表达式IR，返回保存表达式结果的寄存器
static Reg *gen_unaop(Node *node) {
    switch (node->op_type) {
        case OP_NOT: {
            Reg *r0 = gen_expr(node->body);
            printf("not t%d,t%d\n", r0->vn, r0->vn);
            return r0;
        }
        // case OP_BITNOT: {
        //     Reg *r0 = gen_expr(node->body);
        //     printf("not t%d,t%d\n", r0->vn, r0->vn);
        //     return r0;
        // }
        case OP_ADDR: {
            Reg *r0 = gen_lval(node->body);
            return r0;
        }
        case OP_DEREF: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->body);
            // char *access_unit = get_lval_access_unit(node->body);
            printf("l%s t%d,0(t%d)\n", get_access_unit(node->type->align), r0->vn, r1->vn);
            kill_reg(r1);
            return r0;
        }
        case OP_PLUS: {
            Reg *r0 = gen_expr(node->body);
            return r0;
        }
        case OP_MINUS: {
            Reg *r0 = gen_expr(node->body);
            printf("negw t%d,t%d\n", r0->vn, r0->vn);
            return r0;
        }
        case OP_INC: {
            Reg *r0 = gen_lval(node->body);
            Reg *r1 = new_reg();
            char *access_unit = get_access_unit(node->type->align);
            printf("l%s t%d,0(t%d)\n", access_unit, r1->vn, r0->vn);
            printf("addi t%d,t%d,1\n", r1->vn, r1->vn);
            printf("s%s t%d,0(t%d)\n", access_unit, r1->vn, r0->vn);
            kill_reg(r0);
            return r1;
            // Reg *r0 = gen_expr(node->body);
            // printf("addi t%d,t%d,1\n", r0->vn, r0->vn);
            // Var *v = lookup(currentScope, node->body->id->value);
            // if (v) {
            //     if (v->is_gval) {
            //         Reg *r1 = new_reg();
            //         // 先将全局变量的地址加载到一个寄存器中保存
            //         printf("la t%d,%s\n", r1->vn, v->name);
            //         // 将计算结果保存到该地址
            //         printf("sw t%d,0(t%d)\n", r0->vn, r1->vn);
            //         kill_reg(r1);
            //     }
            //     else {
            //         // 通过偏移量保存在栈内
            //         printf("sw t%d,-%d(s0)\n", r0->vn, v->offset);
            //     }
            // }
            // return r0;
        }
        case OP_DEC: {
            Reg *r0 = gen_lval(node->body);
            Reg *r1 = new_reg();
            char *access_unit = get_access_unit(node->type->align);
            printf("l%s t%d,0(t%d)\n", access_unit, r1->vn, r0->vn);
            printf("subi t%d,t%d,1\n", r1->vn, r1->vn);
            printf("s%s t%d,0(t%d)\n", access_unit, r1->vn, r0->vn);
            kill_reg(r0);
            return r1;
        }
        case OP_MEMBER: {
            break;
        }
        case OP_ARR_MEMBER: {
            // 对数组运算符的取值运算,根据其base type返回不同结果
            Reg *r0 = new_reg();
            Reg *r1 = gen_lval(node);
            char *access_unit = get_access_unit(node->type->align);
            printf("l%s t%d,0(t%d)\n", access_unit, r0->vn, r1->vn);
            kill_reg(r1);
            return r0;
            // Var *v = lookup(currentScope, node->body->id->value);
            // Reg *r0 = new_reg();
            // Reg *r1 = gen_expr(node->expression);
            // printf("li t%d,%d\n", r0->vn, v->type_size);
            // printf("mul t%d,t%d,t%d\n", r1->vn, r1->vn, r0->vn);
            // Reg *r2 = gen_expr(node->body);
            // printf("add t%d,t%d,t%d\n", r2->vn, r1->vn, r2->vn);
            // printf("lw t%d,0(t%d)\n", r0->vn, r2->vn);
            // kill_reg(r1);
            // kill_reg(r2);
            // return r0;
        }
    }
}

// 生成二元运算表达式IR，返回保存表达式结果的寄存器
static Reg *gen_binop(Node *node) {
    if (!node) return NULL;
    switch(node->op_type) {
        case OP_EQ: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);

            BB *true_ = new_bb();
            BB *false_ = new_bb();
            BB *last = new_bb();
            printf("beq t%d,t%d,.L%d\n", r1->vn, r2->vn, true_->label);
            kill_reg(r1);
            kill_reg(r2);

            // 分支跳转失败后直接来到false块
            jmp(false_);

            currentBB = true_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,1\n", r0->vn);
            jmp(last);

            currentBB = false_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,0\n", r0->vn);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            return r0;
        }
        case OP_NE: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);

            BB *true_ = new_bb();
            BB *false_ = new_bb();
            BB *last = new_bb();
            printf("bne t%d,t%d,.L%d\n", r1->vn, r2->vn, true_->label);
            kill_reg(r1);
            kill_reg(r2);

            jmp(false_);

            currentBB = true_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,1\n", r0->vn);
            jmp(last);

            currentBB = false_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,0\n", r0->vn);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            return r0;
        }
        case OP_LT: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);

            BB *true_ = new_bb();
            BB *false_ = new_bb();
            BB *last = new_bb();
            printf("blt t%d,t%d,.L%d\n", r1->vn, r2->vn, true_->label);
            kill_reg(r1);
            kill_reg(r2);

            jmp(false_);

            currentBB = true_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,1\n", r0->vn);
            jmp(last);

            currentBB = false_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,0\n", r0->vn);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            return r0;
        }
        case OP_GT: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);

            BB *true_ = new_bb();
            BB *false_ = new_bb();
            BB *last = new_bb();
            printf("bgt t%d,t%d,.L%d\n", r1->vn, r2->vn, true_->label);
            kill_reg(r1);
            kill_reg(r2);

            jmp(false_);

            currentBB = true_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,1\n", r0->vn);
            jmp(last);

            currentBB = false_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,0\n", r0->vn);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            return r0;
        }
        case OP_LE: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);

            BB *true_ = new_bb();
            BB *false_ = new_bb();
            BB *last = new_bb();
            printf("ble t%d,t%d,.L%d\n", r1->vn, r2->vn, true_->label);
            kill_reg(r1);
            kill_reg(r2);

            jmp(false_);

            currentBB = true_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,1\n", r0->vn);
            jmp(last);

            currentBB = false_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,0\n", r0->vn);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            return r0;
        }
        case OP_GE: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);

            BB *true_ = new_bb();
            BB *false_ = new_bb();
            BB *last = new_bb();
            printf("bge t%d,t%d,.L%d\n", r1->vn, r2->vn, true_->label);
            kill_reg(r1);
            kill_reg(r2);

            jmp(false_);

            currentBB = true_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,1\n", r0->vn);
            jmp(last);

            currentBB = false_;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,0\n", r0->vn);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            return r0;
        }
        case OP_LOGAND: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            BB *bb1 = new_bb();
            BB *last = new_bb();
            printf("beqz t%d,.L%d\n", r1->vn, bb1->label);
            kill_reg(r1);

            Reg *r2 = gen_expr(node->rhs);
            printf("beqz t%d,.L%d\n", r1->vn, bb1->label);
            kill_reg(r2);

            printf("li t%d,1\n",r0->vn);
            jmp(last);

            currentBB = bb1;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,0\n",r0->vn);
            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();
            return r0;
        }
        case OP_LOGOR: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            BB *bb1 = new_bb();
            BB *last = new_bb();
            printf("bnez t%d,.L%d\n", r1->vn, bb1->label);
            kill_reg(r1);

            Reg *r2 = gen_expr(node->rhs);
            printf("bnez t%d,.L%d\n", r1->vn, bb1->label);
            kill_reg(r2);

            printf("li t%d,0\n",r0->vn);
            jmp(last);

            currentBB = bb1;
            printf(".L%d:\n", currentBB->label);
            printf("li t%d,1\n",r0->vn);
            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();
            return r0;
        }
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV: {
            // 算术运算
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);
            emit(node->op_type, r0, r1, r2);

            kill_reg(r1);
            kill_reg(r2);
            return r0;
        }
        case OP_ASSIGN: {
            Reg *r1 = gen_lval(node->lhs);
            Reg *r0 = gen_expr(node->rhs);
            char *access_unit = get_access_unit(node->type->align);
            printf("s%s t%d,0(t%d)\n", access_unit, r0->vn, r1->vn);
            kill_reg(r1);
            return r0;

            // Reg *r0 = new_reg();
            // Reg *r1 = gen_expr(node->rhs);
            // Var *temp_var = lookup(currentScope, node->lhs->id->value);
            // if (temp_var->is_gval) {
            //     // 先将全局变量的地址加载到一个寄存器中保存
            //     printf("la t%d,%s\n", r0->vn, temp_var->name);
            //     // 将计算结果保存到该地址
            //     printf("sw t%d,0(t%d)\n", r1->vn, r0->vn);
            //     kill_reg(r0);
            //     return r1;
            // }
            // if (temp_var->offset == 0) {
            //     offset += 4;
            //     temp_var->offset = offset;
            // }
            // printf("mv t%d,t%d\n", r0->vn, r1->vn);
            // printf("sw t%d,-%d(s0)\n", r0->vn, temp_var->offset);

            // kill_reg(r1);
            // return r0;
        }         
        default: {
            printf("unknown op type: %d\n", node->op_type);
            exit(1);
        }
    }
}

// 生成expr IR
static Reg *gen_expr(Node *node) {
    if (!node) return NULL;
    switch (node->node_type) {
        case ND_UNARY_EXPR: {
            return gen_unaop(node);
        }
        case ND_BINARY_EXPR:
            return gen_binop(node);
        case ND_TERNARY_EXPR: {
            BB *bb1 = new_bb();
            BB *bb2 = new_bb();
            BB *last = new_bb();

            Reg *r0 = gen_expr(node->test);
            br(r0, bb1, bb2);

            currentBB = bb1;
            printf(".L%d:\n", currentBB->label);
            Reg *r1 = gen_expr(node->then);
            printf("mv t%d,t%d\n", r0->vn, r1->vn);
            kill_reg(r1);
            jmp(last);

            currentBB = bb2;
            printf(".L%d:\n", currentBB->label);
            Reg *r2 = gen_expr(node->els);
            printf("mv t%d,t%d\n", r0->vn, r2->vn);
            kill_reg(r2);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();
            return r0;
        }
        case ND_NUM: {
            Reg *r0 = new_reg();
            printf("li t%d,%s\n", r0->vn, node->token->value);
            return r0;
        }
        case ND_CHAR: {
            Reg *r0 = new_reg();
            printf("li t%d,%d\n", r0->vn, (int)node->token->value[0]);
            return r0;
        }
        case ND_STR: {
            Reg *r0 = new_reg();
            int clabel = new_constant_label();
            printf(".section    .rodata\n");
            printf(".LC%d:\n", clabel);
            printf("    .string\t%s\n", node->token->value);
            printf(".section    .text\n");
            printf("lla t%d,.LC%d\n", r0->vn, clabel);
            return r0;
            break;
        }
        case ND_IDENT: {
            Var *var = lookup(currentScope, node->token->value);
            if (var == NULL) {
                exit(1);
            }
            if (var->is_gval) {
                Reg *r0 = new_reg();
                Reg *r1 = new_reg();
                // 加载全局变量地址
                printf("la t%d,%s\n", r0->vn, var->name);
                // 通过地址加载全局变量的值
                printf("l%s t%d,0(t%d)\n", get_access_unit(var->type->align), r1->vn, r0->vn);
                kill_reg(r0);
                return r1;
            }
            else if (var->type->kind == TY_ARRAY_OF) {
                // 返回数组首地址
                Reg *r0 = new_reg();
                if (!var->is_gval) {
                    if (var->is_param) {
                        // printf("addi t%d,s0,%d\n", r0->vn, var->offset);
                        // printf("ld t%d,0(t%d)\n", r0->vn, r0->vn);
                        printf("ld t%d,%d(s0)\n", r0->vn, var->offset);
                    }
                    else {
                        // printf("ld t%d,%d(s0)\n", r0->vn, var->offset);
                        printf("addi t%d,s0,%d\n", r0->vn, var->offset);
                    }
                }
                else {
                    printf("la t%d,%s\n", r0->vn, var->name);
                }
                return r0;
            }
            else if (var->type->kind == TY_POINTER_TO) {
                Reg *r0 = new_reg();
                printf("ld t%d,%d(s0)\n", r0->vn, var->offset);
                return r0;
            }
            else if (var->type->kind == TY_FUNC) {
                Reg *r0 = new_reg();
                printf("la t%d,%s\n", r0->vn, var->type->name);
                return r0;
            }
            else {
                Reg *r0 = new_reg();
                printf("l%s t%d,%d(s0)\n", get_access_unit(var->type->align), r0->vn, var->offset);
                return r0;
            }
        }
        case ND_CALLEXPR: {
            Node *callee = node->callee;
            Reg *bak = NULL;
            if (callee->node_type == ND_UNARY_EXPR
            || callee->node_type == ND_CALLEXPR
            ) {
                bak = gen_expr(node->callee);
            }
            
            Reg *args[6];
            // 先依次计算出函数实参
            for (int i = 0;i < node->args->len;i++) {
                Node *param = node->args->data[i];
                args[i] = gen_expr(param);
            }
            // 将实参依次带入参数寄存器
            for (int i = 0;i < node->args->len;i++) {
                printf("mv a%d,t%d\n", i, args[i]->vn);
            }

            if (callee->node_type == ND_UNARY_EXPR
                || callee->node_type == ND_CALLEXPR
            ) {
                printf("jalr t%d\n", bak->vn);
                kill_reg(bak);
            }

            if (callee->node_type == ND_IDENT) {
                // if (callee->type
                //     && (
                //         callee->type == TY_POINTER_TO
                //         || callee->type == TY_FUNC
                //     ) 
                // ) {
                    
                // }
                // else {
                    printf("call %s\n", node->callee->token->value);
                // }
            }

            // 保存返回值
            Reg *r0 = new_reg();
            printf("mv t%d,a0\n", r0->vn);

            // 函数调用结束，释放寄存器
            for (int i = 0;i < node->args->len;i++) {
                kill_reg(args[i]);
            }
            return r0;
        }
    }
}

char *to_str(int op) {
    switch (op) {
        case OP_ADD: return ("add");
        case OP_SUB: return ("sub");
        case OP_MUL: return ("mul");
        case OP_DIV: return ("div");
        default:
            return "NULL";
    }
}

static void emit(int op, Reg *r0, Reg *r1, Reg *r2) {
    printf("%s t%d,t%d,t%d\n", to_str(op), r0->vn, r1->vn, r2->vn);
    IR *ir = new_ir();
    ir->op = op;
    ir->r0 = r0;
    ir->r1 = r1;
    ir->r2 = r2;
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
// 本函数的返回值是一个保存了该左值地址的寄存器结构体

/**
 * C语言中有哪些左值？
 * 1.指针
 * 2.数组元素(非数组名)
 * 3.变量(int/char)
*/
static Reg *gen_lval(Node *node) {
    switch(node->node_type) {
        case ND_IDENT: {
            // 如果当前是ident,则有两种情况:
            // 全局变量(变量/函数/指针/数组),均是从标签加载地址
            // 局部变量
            //      1.是函数参数
            //      2.是函数本地变量
            // 不考虑数组情况，数组情况在ND_UNARY_EXPR情况中处理，只考虑基本类型的变量
            Var *var = lookup(currentScope, node->token->value);
            if (!var) {
                printf("no var found: %s\n", node->token->value);
            }
            Reg *r0 = new_reg();
            // 全局变量,直接从标签加载地址
            if (var->is_gval) {
                printf("la t%d,%s\n", r0->vn, var->name);
            }
            else {
                // 局部变量,如果是指针且函数参数(说明变量不在此声明,var->offset为间接地址)
                // 此时真正的地址在var->offset(s0)内
                if (var->type->kind == TY_POINTER_TO && var->is_param) {
                    printf("ld t%d,%d(s0)\n", r0->vn, var->offset);
                }
                // 局部变量,且声明在此函数内,var->offset为直接地址
                else {
                    printf("addi t%d,s0,%d\n", r0->vn, var->offset);
                }
            }
            return r0;
        }
        case ND_UNARY_EXPR: {
            switch(node->op_type) {
                case OP_DEREF: {
                    // 解引用(指针运算符)，返回指针变量的值(即指向元素的地址)
                    return gen_expr(node->body);
                }
                case OP_MEMBER: {
                    break;
                }
                case OP_ARR_MEMBER: {
                    // 数组取元素运算符,将其视为指针处理

                    Reg *r1 = gen_expr(node->expression);
                    // 根据数组下标计算数组元素地址
                    // addr(array[i]) = arr + i * arr->type->size ? arr->type->align 待确认
                    Reg *r2 = new_reg();
                    printf("li t%d,%d\n", r2->vn, node->type->size);
                    Reg *r3 = new_reg();
                    printf("mul t%d,t%d,t%d\n", r3->vn, r2->vn, r1->vn);
                    kill_reg(r2);
                    kill_reg(r1);

                    // Reg *r0 = gen_expr(node->body);
                    Reg *r0 = gen_lval(node->body);
                    printf("add t%d,t%d,t%d\n", r0->vn, r0->vn, r3->vn);
                    kill_reg(r3);
                    return r0;

                    // // 加载数组首地址
                    // Reg *r0 = new_reg();
                    // if (var->is_gval) {
                    //     printf("la t%d,%s\n", r0->vn, var->name);
                    // }
                    // else if (var->is_param) {
                    //     // printf("addi t%d,s0,%d\n", r0->vn, var->offset);
                    //     // printf("ld t%d,0(t%d)\n", r0->vn, r0->vn);
                    //     printf("ld t%d,%d(s0)\n", r0->vn, var->offset);
                    // }
                    // else {
                    //     printf("addi t%d,s0,%d\n", r0->vn, var->offset);
                    // }
                    // printf("add t%d,t%d,t%d\n", r0->vn, r0->vn, r1->vn);
                }
            }
        }
    }
}

// 获取左值变量访问标识符
// static char *get_lval_access_unit(Node *node) {
//     switch(node->node_type) {
//         case ND_IDENT: {
//             Var *var = lookup(currentScope, node->token->value);
//             return get_access_unit(var->type->align);
//         }
//         case ND_UNARY_EXPR: {
//             switch(node->op_type) {
//                 case OP_DEREF: {
//                     Var *var = lookup(currentScope, node->body->id->value);
//                     return get_access_unit(var->type->align);
//                 }
//                 case OP_MEMBER: {
//                     break;
//                 }
//                 case OP_ARR_MEMBER: {
//                     Var *var = lookup(currentScope, node->body->id->value);
//                     return get_access_unit(var->type->align);
//                 }
//             }
//         }
//     }
// }

// 对于每一个basic block,确认好它的输入输出（如：有几个输入？几个输出？输入和输出在基本块的什么位置？）
// 生成中间代码的第一步，好像还用不到basic block,basic block的主要作用是在后续优化
// 那现在离开basic block怎么生成代码？
// 在该生成basic block的地方顺序打印ir,生成basic block前先生成该块的前置块和后置块，然后basic block从入口到出口顺序执行
static void gen_stmt(Node *node) {
    if (node == NULL) return;
    switch (node->node_type) {
        case ND_BLOCK: {
            // block_stmt直接生成一个新的basic block,basic block中依次存放所有子stmt的ir
            // BB *body = new_bb();
            // currentBB = body;
            currentScope = node->scope;
            for (int i = 0;i < node->stmts->len;i++) {
                gen_stmt(node->stmts->data[i]);
            }
            currentScope = exitScope(currentScope);
            break;
        }
        case ND_WHILE_STMT: {
            // 对于while stmt,将其分成两个基本块：test bb,body bb,但是还涉及到一个去向 bb
            // 如何处理去向bb,我的想法是顺序生成bb,while stmt的去向bb就是下一个bb;
            BB *test = new_bb();
            BB *body = new_bb(); // while_stmt的主体
            BB *last = new_bb(); // while_stmt break后的去处

            BB *break_save = break_;
            BB *continue_save = continue_;
            break_ = last;
            continue_ = test;

            currentBB = test;
            printf(".L%d:\n", currentBB->label);
            Reg *r0 = gen_expr(node->test);
            br(r0, body, last);
            kill_reg(r0);

            currentBB = body;
            printf(".L%d:\n", currentBB->label);
            gen_stmt(node->body);
            jmp(test);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            break_ = break_save;
            continue_ = continue_save;
            break;
        }
        case ND_DO_WHILE: {
            BB *body = new_bb();
            BB *test = new_bb();
            BB *last = new_bb();

            BB *break_save = break_;
            BB *continue_save = continue_;
            break_ = last;
            continue_ = test;

            currentBB = body;
            printf(".L%d:\n", currentBB->label);
            gen_stmt(node->body);

            currentBB = test;
            printf(".L%d:\n", currentBB->label);
            Reg *r0 = gen_expr(node->test);
            br(r0, body, last);
            kill_reg(r0);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            break_ = break_save;
            continue_ = continue_save;
            break;
        }
        case ND_FOR_STMT: {
            // 对于for stmt,和while一样分成两个主要basic block,test bb, body && update bb,以及去向bb
            // for stmt的开头还需要一个init bb,这样for stmt比while stmt多一个bb,应该是三个bb
            BB *init = new_bb();
            BB *test = new_bb();
            BB *update = new_bb();
            BB *body = new_bb();
            BB *last = new_bb();

            BB *break_save = break_;
            BB *continue_save = continue_;
            break_ = last;
            continue_ = update;
            
            currentBB = init;
            printf(".L%d:\n", currentBB->label);
            if (node->init) {
                Reg *init_reg = gen_expr(node->init);
                jmp(test);
                kill_reg(init_reg);
            }

            currentBB = test;
            printf(".L%d:\n", currentBB->label);
            if (node->test) {
                Reg *test_reg = gen_expr(node->test);
                br(test_reg, body, last);
                kill_reg(test_reg);
            }

            currentBB = body;
            printf(".L%d:\n", currentBB->label);
            gen_stmt(node->body);

            currentBB = update;
            printf(".L%d:\n", currentBB->label);
            if (node->update) {
                Reg *update_reg = gen_expr(node->update);
                kill_reg(update_reg);
            }
            jmp(test);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();

            break_ = break_save;
            continue_ = continue_save;
            break;
        }
        case ND_IF_STMT: {
            BB *test = new_bb();
            BB *then = new_bb();
            BB *els = new_bb();
            BB *last = new_bb();

            currentBB = test;
            printf(".L%d:\n", currentBB->label);
            Reg *r0 = gen_expr(node->test);
            br(r0, then, els);
            kill_reg(r0);

            currentBB = then;
            printf(".L%d:\n", currentBB->label);
            gen_stmt(node->then);
            jmp(last);

            currentBB = els;
            printf(".L%d:\n", currentBB->label);
            gen_stmt(node->els);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();
            break;
        }
        case ND_SWITCH_STMT: {
            // r0保存test的值
            Reg *r0 = gen_expr(node->test);
            Vector *caseBBs = new_vec();
            // break后的去处
            BB *last = new_bb();

            BB *break_save = break_;
            break_ = last;
            // 先根据test生成跳转列表，一个case语句对应一个basic block
            for (int i = 0;i < node->cases->len;i++) {
                Node *case_ = node->cases->data[i];
                BB *caseBB = new_bb();
                vec_push(caseBBs, caseBB);

                if (case_->test == NULL) continue;
                Reg *r1 = gen_expr(case_->test);
                printf("beq t%d,t%d,.L%d\n", r0->vn, r1->vn, caseBB->label);
                kill_reg(r1);
            }
            kill_reg(r0);
            // 如果有default分支，则switch的test结束后添加跳转到default分支，而不是跳转到结束分支
            for (int i = 0;i < node->cases->len;i++) {
                Node *case_ = node->cases->data[i];
                if (case_->test == NULL) {
                    jmp(caseBBs->data[i]);
                    break;
                }
                else if (i == node->cases->len) {
                    jmp(last);
                }
            }
            // 逐个生成case基本块
            for (int i = 0;i < node->cases->len;i++) {
                Node *case_ = node->cases->data[i];
                currentBB = caseBBs->data[i];
                printf(".L%d:\n", currentBB->label);
                gen_stmt(case_->then);
            }
            // 生成最后的last basic block
            currentBB = last;
            printf(".L%d:\n", currentBB->label);

            break_ = break_save;
            break;
        }
        case ND_CASE: {
            break;
        }
        case ND_CONTINUE_STMT: {
            jmp(continue_);
            break;
        }
        case ND_BREAK_STMT: {
            jmp(break_);
            break;
        }
        case ND_EXPR_STMT: {
            Reg *r0 = gen_expr(node->body);
            kill_reg(r0);
            break;
        }
        case ND_RETURN_STMT: {
            // 如果return语句不含返回值，则直接return,否则将计算返回值并将返回值移动至返回值寄存器
            if (node->body != NULL) {
                Reg *r0 = gen_expr(node->body);
                printf("mv a0,t%d\n",r0->vn);
                kill_reg(r0);
            }
            jmp(returnBB);
            break;
        }
    }
}

static void gen_param(Var *var, int i) {
    // offset -= var->type->size;
    // var->offset = offset;

    Reg *r0 = new_reg();
    printf("mv t%d,a%d\n", r0->vn, i);
    if (var->type->kind == TY_POINTER_TO || var->type->kind == TY_ARRAY_OF) {
        printf("sd t%d,%d(s0)\n", r0->vn, var->offset);
    }
    else printf("s%s t%d,%d(s0)\n", get_access_unit(var->type->size), r0->vn, var->offset);
    kill_reg(r0);
}

int compute_function_stack_size(Function *func) {    
    int count_size = 16; // 预留16byte空间存s0和ra
    count_size += (8 * 7); // 预留保存寄存器的值
    // 函数形参
    for (int i = 0;i < func->params->len;i++) {
        Var *var = func->params->data[i];
        count_size += var->type->size;
    }
    // 函数局部变量
    for (int i = 0;i < func->lvars->len;i++) {
        Var *var = func->lvars->data[i];
        count_size += var->type->size;
    }
    if (count_size % 16 != 0) { // 16字节对齐
        count_size = ((count_size / 16) + 1) * 16;
    }
    return count_size;
}

void emit_data(Var *var) {
    gen_gvar(var);
}

void emit_code(Function *fn) {
    // 函数代码生成前计算函数栈需要多少空间
    int stack_size = compute_max_size(currentScope);

    stack_size += 72;

    if (stack_size % 16 != 0) stack_size = ((stack_size / 16) + 1) * 16;

    // 重置offset值为-16(0 - ra - s0)
    // offset = -16;

    printf("%s:\n", fn->name);

    printf("addi    sp,sp,-%d\n", stack_size);
    printf("sd      ra,%d(sp)\n", stack_size - 8);
    printf("sd      s0,%d(sp)\n", stack_size - 16);
    printf("addi    s0,sp,%d\n", stack_size);

    printf("sd      t0,-%d(s0)\n", 16 + 8 * 1);
    printf("sd      t1,-%d(s0)\n", 16 + 8 * 2);
    printf("sd      t2,-%d(s0)\n", 16 + 8 * 3);
    printf("sd      t3,-%d(s0)\n", 16 + 8 * 4);
    printf("sd      t4,-%d(s0)\n", 16 + 8 * 5);
    printf("sd      t5,-%d(s0)\n", 16 + 8 * 6);
    printf("sd      t6,-%d(s0)\n", 16 + 8 * 7);

    // offset -= 56;

    puts("");

    returnBB = new_bb();

    for (int i = 0;i < fn->params->len;i++) {
        gen_param(fn->params->data[i], i);
    }
    for (int i = 0;i < fn->lvars->len;i++) {
        gen_lvar(fn->lvars->data[i]);
    }
    for (int i = 0;i < fn->stmts->len;i++) {
        gen_stmt(fn->stmts->data[i]);
    }

    puts("");

    
    currentBB = returnBB;
    printf(".L%d:\n", currentBB->label);

    printf("ld      t0,-%d(s0)\n", 16 + 8 * 1);
    printf("ld      t1,-%d(s0)\n", 16 + 8 * 2);
    printf("ld      t2,-%d(s0)\n", 16 + 8 * 3);
    printf("ld      t3,-%d(s0)\n", 16 + 8 * 4);
    printf("ld      t4,-%d(s0)\n", 16 + 8 * 5);
    printf("ld      t5,-%d(s0)\n", 16 + 8 * 6);
    printf("ld      t6,-%d(s0)\n", 16 + 8 * 7);

    printf("ld      ra,%d(sp)\n", stack_size - 8);
    printf("ld      s0,%d(sp)\n", stack_size - 16);
    printf("addi    sp,sp,%d\n", stack_size);
    printf("jr      ra\n");
}

static void init_registers() {
    registers = new_vec();
    for (int i = 0;i <= 6;i++) {
        vec_push(registers, new_real_reg());
    }
}

static void reset_registers(Vector *regs) {
    for (int i = 0;i < regs->len;i++) {
        Reg *r = regs->data[i];
        r->using = false;
    }
}

void codegen(Program *prog, SymbolTable *table) {
    break_ = NULL;
    continue_ = NULL;

    init_registers();
    check_type(prog, table);

    // .rodata只读，向.rodata段写会报错，因此可修改全局变量应保存在.data段
    printf(".section	.data\n");
    for (int i = 0;i < prog->gvars->len;i++) {
        emit_data(prog->gvars->data[i]);
        puts("");
    }
    printf(".section	.text\n");
    puts("");
    for (int i = 0;i < prog->funcs->len;i++) {
        reset_registers(registers);
        Function *fn = prog->funcs->data[i];
        currentFunction = fn;
        currentScope = table->children->data[i];

        printf(".globl	%s\n", fn->name);
        printf(".type	%s, @function\n", fn->name);
        emit_code(fn);
        puts("");
    }
}