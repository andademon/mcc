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
static IR *br(Reg *r, BB *then, BB *els);
static IR *jmp(BB *bb);

int nlabel = 1;
int nreg = 0;
int clabel = 0;

int offset = 0;

static SymbolTable *table;

static SymbolTable *currentScope;
static Function *currentFunction; 
static BB *currentBB; // 指向当前basic block
static BB *returnBB; // 保存函数返回时的basic block, 于函数初始化时同时初始化
static BB *break_; // 临时basic block, 用于指向break语句的返回block, codegen初始化为NULL
static BB *continue_; // 临时basic block, 用于指向continue语句的返回block, codegen初始化为NULL

static Vector *registers;

static void nop() {
    printf("nop\n\n");
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

// 将局部变量保存在内存中，返回记录的偏移地址（Q: 偏移地址如何计算？）
static void gen_lvar(Var *var) {
    // 无初始化，跳过
    // 有初始化
    // 将初始化结果加载到寄存器，然后保存到内存，在符号表中记录在内存中的偏移量

    // 当且仅当变量有赋初值时
    if (var->init == NULL) return;

    // 计算新offset并更新到符号表中
    offset += 4;
    var->offset = offset;
    Var *temp_var = lookup(currentScope, var->name);
    temp_var->offset = offset;

    // 将初始化结果保存到内存
    Reg *reg = gen_expr(var->init);
    switch(var->type) {
        case INT: {
            printf("sw t%d,-%d(s0)\n", reg->vn, offset);
            break;
        }
        case CHAR: {
            printf("sb t%d,-%d(s0)\n", reg->vn, offset);
            break;
        }
    }
    kill_reg(reg);
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
            // printf("sub t%d,t%d,t%d\n", r0->vn, r1->vn, r2->vn);
            // Reg *r3 = new_reg();
            // printf("seqz t%d,t%d\n", r3->vn, r0->vn);

            // kill_reg(r0);
            // kill_reg(r1);
            // kill_reg(r2);
            // return r3;
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
            // Reg *r0 = new_reg();
            // Reg *r1 = gen_expr(node->lhs);
            // Reg *r2 = gen_expr(node->rhs);
            // printf("sub t%d,t%d,t%d\n", r0->vn, r1->vn, r2->vn);
            // Reg *r3 = new_reg();
            // printf("snez t%d,t%d\n", r3->vn, r0->vn);

            // kill_reg(r0);
            // kill_reg(r1);
            // kill_reg(r2);
            // return r3;
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
            // Reg *r0 = new_reg();
            // Reg *r1 = gen_expr(node->lhs);
            // Reg *r2 = gen_expr(node->rhs);
            // printf("slt t%d,t%d,t%d\n", r0->vn, r1->vn, r2->vn);

            // kill_reg(r1);
            // kill_reg(r2);
            // return r0;
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
            // Reg *r0 = new_reg();
            // Reg *r1 = gen_expr(node->lhs);
            // Reg *r2 = gen_expr(node->rhs);
            // printf("slt t%d,t%d,t%d\n", r0->vn, r2->vn, r1->vn);

            // kill_reg(r1);
            // kill_reg(r2);
            // return r0;
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
            // Reg *r0 = new_reg();
            // Reg *r1 = gen_expr(node->lhs);
            // Reg *r2 = gen_expr(node->rhs);
            // printf("sub t%d,t%d,t%d\n", r0->vn, r1->vn, r2->vn);

            // kill_reg(r1);
            // kill_reg(r2);

            // Reg *r3 = new_reg();
            // Reg *r4 = new_reg();
            // Reg *r5 = new_reg();
            // printf("sltz t%d,t%d\n", r3->vn, r0->vn);
            // printf("seqz t%d,t%d\n", r4->vn, r0->vn);
            // printf("or t%d,t%d,t%d\n", r5->vn, r3->vn, r4->vn);

            // kill_reg(r0);
            // kill_reg(r3);
            // kill_reg(r4);
            // return r5;
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
            // Reg *r0 = new_reg();
            // Reg *r1 = gen_expr(node->lhs);
            // Reg *r2 = gen_expr(node->rhs);
            // printf("sub t%d,t%d,t%d\n", r0->vn, r2->vn, r1->vn);

            // kill_reg(r1);
            // kill_reg(r2);

            // Reg *r3 = new_reg();
            // Reg *r4 = new_reg();
            // Reg *r5 = new_reg();
            // printf("sltz t%d,t%d\n", r3->vn, r0->vn);
            // printf("seqz t%d,t%d\n", r4->vn, r0->vn);
            // printf("or t%d,t%d,t%d\n", r5->vn, r3->vn, r4->vn);

            // kill_reg(r0);
            // kill_reg(r3);
            // kill_reg(r4);
            // return r5;
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
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->rhs);
            Var *temp_var = lookup(currentScope, node->lhs->id->value);
            if (temp_var->is_gval) {
                // 先将全局变量的地址加载到一个寄存器中保存
                printf("la t%d,%s\n", r0->vn, temp_var->name);
                // 将计算结果保存到该地址
                printf("sw t%d,0(t%d)\n", r1->vn, r0->vn);
                kill_reg(r0);
                return r1;
            }
            if (temp_var->offset == 0) {
                offset += 4;
                temp_var->offset = offset;
            }
            printf("mv t%d,t%d\n", r0->vn, r1->vn);
            printf("sw t%d,-%d(s0)\n", r0->vn, temp_var->offset);

            kill_reg(r1);
            return r0;
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
            Reg *r1 = gen_expr(node->consequent);
            printf("mv t%d,t%d\n", r0->vn, r1->vn);
            kill_reg(r1);
            jmp(last);

            currentBB = bb2;
            printf(".L%d:\n", currentBB->label);
            Reg *r2 = gen_expr(node->alternative);
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
            printf("li t%d,%s\n", r0->vn, node->tok->value);
            return r0;
        }
        case ND_CHAR: {
            Reg *r0 = new_reg();
            printf("li t%d,%d\n", r0->vn, (int)node->tok->value[0]);
            return r0;
        }
        case ND_STR: {
            Reg *r0 = new_reg();
            int clabel = new_constant_label();
            printf(".section    .rodata\n");
            printf(".LC%d:\n", clabel);
            printf("    .string\t%s\n", node->tok->value);
            printf(".section    .text\n");
            printf("lla t%d,.LC%d\n", r0->vn, clabel);
            return r0;
            break;
        }
        case ND_IDENT: {
            Var *v = lookup(currentScope, node->id->value);
            if (v == NULL) {
                exit(1);
            }
            if (v->is_gval) {
                Reg *r0 = new_reg();
                Reg *r1 = new_reg();
                // 加载全局变量地址
                printf("la t%d,%s\n", r0->vn, v->name);
                // 通过地址加载全局变量的值
                printf("lw t%d,0(t%d)\n", r1->vn, r0->vn);
                kill_reg(r0);
                return r1;
            }
            else {
                Reg *r0 = new_reg();
                printf("lw t%d,-%d(s0)\n", r0->vn, v->offset);
                return r0;
            }
        }
        case ND_FUNCALL: {
            Reg *args[6];
            // 对于函数参数的处理，如果是立即数，直接li加载，否则（是左值）看arg在哪里保存
            for (int i = 0;i < node->args->len;i++) {
                Node *param = node->args->data[i];
                args[i] = gen_expr(param);
            }
            for (int i = 0;i < node->args->len;i++) {
                printf("mv a%d,t%d\n", i, args[i]->vn);
            }
            Reg *r0 = new_reg();
            printf("call %s\n", node->id->value);
            printf("mv t%d,a0\n", r0->vn);

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
        case OP_EQ: return ("");
        case OP_NE: return ("");
        case OP_LT: return ("");
        case OP_LE: return ("");
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
static void gen_lval(Node *node) {
    if (node->node_type != ND_IDENT) return;
    // offset += 4;
    Var *var = lookup(currentScope, node->id->value);
    if (var->offset != 0) {
        // printf("lw t%d,-%d(s0)\n",);
    }
    var->offset = offset;
    Reg *r0 = new_reg();
    // printf("mv t%d,a%d\n", r0->vn, i);
    // printf("sw t%d,-%d(s0)\n", r0->vn, offset);
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
            for (int i = 0;i < node->stmts->len;i++) {
                gen_stmt(node->stmts->data[i]);
            }
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
            gen_stmt(node->consequent);
            jmp(last);

            currentBB = els;
            printf(".L%d:\n", currentBB->label);
            gen_stmt(node->alternative);
            jmp(last);

            currentBB = last;
            printf(".L%d:\n", currentBB->label);
            nop();
            break;
        }
        case ND_SWITCH_STMT: {
            // r0保存discriminant的值
            Reg *r0 = gen_expr(node->discriminant);
            Vector *caseBBs = new_vec();
            // break后的去处
            BB *last = new_bb();

            BB *break_save = break_;
            break_ = last;
            // 先根据discriminant生成跳转列表，一个case语句对应一个basic block
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
            // 如果有default分支，则switch的discriminant结束后添加跳转到default分支，而不是跳转到结束分支
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
                gen_stmt(case_->consequent);
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
    offset += 4;
    Reg *r0 = new_reg();
    var->offset = offset;
    Var *temp_var = lookup(currentScope, var->name);
    temp_var->offset = offset;
    printf("mv t%d,a%d\n", r0->vn, i);
    printf("sw t%d,-%d(s0)\n", r0->vn, offset);
    kill_reg(r0);
}

int compute_var_size(Var *var) {
    int size = get_size(var->type);
    size = (size < 4) ? 4 :size;
    if (var->is_array) return size * var->len;
    return size;
}

int compute_function_stack_size(Function *func) {    
    int count_size = 16; // 预留16byte空间存s0和ra
    count_size += (8 * 7); // 预留保存寄存器的值
    for (int i = 0;i < func->lvars->len;i++) {
        Var *var = func->lvars->data[i];
        count_size += compute_var_size(var);
    }
    if (count_size % 16 != 0) {
        count_size = ((count_size / 16) + 1) * 16;
    }
    return count_size;
}

void emit_data(Var *var) {
    gen_gvar(var);
}

void emit_code(Function *fn) {    
    // 函数代码生成前计算函数栈需要多少空间
    int stack_size = compute_function_stack_size(fn);

    offset = 16;

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

    offset += 56;

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

static void reset_registers(Vector *regs) {
    for (int i = 0;i < regs->len;i++) {
        Reg *r = regs->data[i];
        r->using = false;
    }
}

void codegen(Program *prog) {
    break_ = NULL;
    continue_ = NULL;
    if (!table) {
        table = buildSymbolTable(prog);
        currentScope = table;
    }
    if (!registers) {
        registers = new_vec();
        for (int i = 0;i <= 6;i++) {
            vec_push(registers, new_real_reg());
        }
    }
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
        currentScope = currentScope->children->data[i];

        printf(".globl	%s\n", fn->name);
        printf(".type	%s, @function\n", fn->name);
        emit_code(fn);
        puts("");
        currentScope = exitScope(currentScope);
    }
}

int compute_offset(Var *var) {

}