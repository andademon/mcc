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
static IR *jmp(BB *bb);

int nlabel = 1;
int nreg = 1;
int clabel = 0;

static int offset = 0;

static SymbolTable *table;

static SymbolTable *currentScope;

static Function *currentFunction; 
static BB *currentBB;

static int new_label() {
    return nlabel++;
}

static int new_constant_label() {
    return clabel++;
}

static Reg *new_reg() {
    Reg *r = calloc(1, sizeof(Reg));
    r->vn = nreg++;
    r->rn = -1;
    return r;
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
    // 有初始化，且初始化类型为以下之一：
    // INT | CHAR | BOOL
    // 加载到寄存器，然后保存到内存，在符号表中记录在内存中的偏移量 -> li，sw

    // 当且仅当变量有赋初值且为常量赋值时
    if (var->init == NULL 
        || (var->init->node_type != ND_NUM 
            && var->init->node_type != ND_CHAR)
    )
        return;

    Reg *reg = new_reg();
    var->offset = offset;
    Var *temp_var = lookup(currentScope, var->name);
    temp_var->offset = offset;
    switch(var->type) {
        case INT: {
            printf("li r%d,%s\n", reg->vn, var->init->tok->value);
            printf("sw r%d,-%d(s0)\n", reg->vn, offset);
            break;
        }
        case CHAR: {
            printf("li r%d,%d\n", reg->vn, (int)var->init->tok->value[0]);
            printf("sb r%d,-%d(s0)\n", reg->vn, offset);
            break;
        }
    }
    offset += 8;
}

// 生成二元运算表达式IR
static Reg *gen_binop(Node *node) {
    if (!node) return NULL;
    switch(node->op_type) {
        case OP_EQ: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);
            printf("sub r%d,r%d,r%d\n", r0->vn, r1->vn, r2->vn);
            Reg *r3 = new_reg();
            printf("seqz r%d,r%d\n", r3->vn, r0->vn);
            return r3;
        }
        case OP_NE: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);
            printf("sub r%d,r%d,r%d\n", r0->vn, r1->vn, r2->vn);
            Reg *r3 = new_reg();
            Reg *r4 = new_reg();
            Reg *r5 = new_reg();
            printf("snez r%d,r%d\n", r3->vn, r0->vn);
            return r3;
        }
        case OP_LT: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);
            printf("slt r%d,r%d,r%d\n", r0->vn, r1->vn, r2->vn);
            return r0;
        }
        case OP_LT2: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);
            printf("slt r%d,r%d,r%d\n", r0->vn, r2->vn, r1->vn);
            return r0;
        }
        case OP_LE: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);
            printf("sub r%d,r%d,r%d\n", r0->vn, r1->vn, r2->vn);
            Reg *r3 = new_reg();
            Reg *r4 = new_reg();
            Reg *r5 = new_reg();
            printf("sltz r%d,r%d\n", r3->vn, r0->vn);
            printf("seqz r%d,r%d\n", r4->vn, r0->vn);
            printf("or r%d,r%d,r%d\n", r5->vn, r3->vn, r4->vn);
            return r5;
        }
        case OP_LE2: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->lhs);
            Reg *r2 = gen_expr(node->rhs);
            printf("sub r%d,r%d,r%d\n", r0->vn, r2->vn, r1->vn);
            Reg *r3 = new_reg();
            Reg *r4 = new_reg();
            Reg *r5 = new_reg();
            printf("sltz r%d,r%d\n", r3->vn, r0->vn);
            printf("seqz r%d,r%d\n", r4->vn, r0->vn);
            printf("or r%d,r%d,r%d\n", r5->vn, r3->vn, r4->vn);
            return r5;
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
            break;
        case ND_NUM: {
            Reg *r0 = new_reg();
            printf("li r%d,%s\n", r0->vn, node->tok->value);
            return r0;
            break;
        }
        case ND_STR: {
            printf(".section    .rodata\n");
            printf(".LC%d:\n", new_constant_label());
            printf("\t.string\t%s\n", node->tok->value);
            printf(".section    .text\n");
            return NULL;
            break;
        }
        case ND_IDENT: {
            Var *v = lookup(currentScope, node->id->value);
            if (v == NULL) {
                exit(1);
            }
            Reg *r0 = new_reg();
            printf("lw r%d,-%d(s0)\n", r0->vn, v->offset);
            return r0;
            break;
        }
        case ND_ASSIGN_EXPR: {
            Reg *r0 = new_reg();
            Reg *r1 = gen_expr(node->rhs);
            Var *temp_var = lookup(currentScope, node->lhs->id->value);
            if (temp_var->offset == 0) {
                offset += 4;
                temp_var->offset = offset;
            }
            printf("mv r%d,r%d\n", r0->vn, r1->vn);
            printf("sw r%d,-%d(s0)\n", r0->vn, temp_var->offset);
            break;
        }
        case ND_FUNCALL: {
            Reg *args[6];
            // 对于函数参数的处理，如果是立即数，直接li加载，否则（是左值）看arg在哪里保存
            for (int i = 0;i < node->args->len;i++) {
                Node *param = node->args->data[i];
                if (param->node_type == ND_STR) {
                    int clabel = new_constant_label();
                    printf(".section    .rodata\n");
                    printf(".LC%d:\n", clabel);
                    printf("    .string\t%s\n", param->tok->value);
                    printf(".section    .text\n");
                    printf("lla a%d,.LC%d\n", i, clabel);
                    continue;
                }
                args[i] = gen_expr(param);
                printf("mv a%d,r%d\n", i, args[i]->vn);
            }
            Reg *r0 = new_reg();
            printf("call %s\n", node->id->value);
            printf("mv r%d, a0\n", r0->vn);
            return r0;
            break;
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
    printf("%s r%d,r%d,r%d\n", to_str(op), r0->vn, r1->vn, r2->vn);
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
        // printf("lw r%d,-%d(s0)\n",);
    }
    var->offset = offset;
    Reg *r0 = new_reg();
    // printf("mv r%d,a%d\n", r0->vn, i);
    // printf("sw r%d,-%d(s0)\n", r0->vn, offset);
}

// 分支跳转IR,绑定真分支与跳转分支
static IR *br(Reg *r, BB *then, BB *els) {
    printf("beq r%d,zero,.L%d\n", r->vn, els->label);
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
            BB *body = new_bb();
            currentBB = body;
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
            BB *break_ = new_bb(); // while_stmt break后的去处

            currentBB = test;
            printf(".L%d:\n", currentBB->label);
            br(gen_expr(node->test), body, break_);

            currentBB = body;
            printf(".L%d:\n", currentBB->label);
            gen_expr(node->body);
            jmp(test);

            break;
        }
        case ND_FOR_STMT: {
            // 对于for stmt,和while一样分成两个主要basic block,test bb, body && update bb,以及去向bb
            // for stmt的开头还需要一个init bb,这样for stmt比while stmt多一个bb,应该是三个bb
            BB *init = new_bb();
            BB *body = new_bb();
            BB *test = new_bb();
            BB *break_ = new_bb();
            
            currentBB = init;
            printf(".L%d:\n", currentBB->label);
            if (node->init) {
                gen_expr(node->init);
                jmp(test);
            }

            currentBB = test;
            printf(".L%d:\n", currentBB->label);
            if (node->test) {
                Reg *r = gen_expr(node->test);
                br(r, body, break_);
            }

            currentBB = body;
            printf(".L%d:\n", currentBB->label);
            gen_stmt(node->body);

            if (node->update)
                gen_expr(node->update);

            // 这里body的出口是jmp test吗？有点不确定，待会验证            
            jmp(test);
            break;
        }
        case ND_IF_STMT: {
            BB *test = new_bb();
            BB *then = new_bb();
            BB *els = new_bb();
            BB *last = new_bb();

            currentBB = test;
            printf(".L%d:\n", currentBB->label);
            br(gen_expr(node->test), then, els);

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
            break;
        }
        case ND_EXPR_STMT: {
            gen_expr(node->body);
            break;
        }
        case ND_RETURN_STMT: {
            // 如果return语句不含返回值，则直接return,否则将计算返回值并将返回值移动至返回值寄存器
            if (node->body == NULL) return;
            Reg *reg = gen_expr(node->body);
            printf("mv a0,r%d\n",reg->vn);
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
    printf("mv r%d,a%d\n", r0->vn, i);
    printf("sw r%d,-%d(s0)\n", r0->vn, offset);
}

int compute_var_size(Var *var) {
    int size = get_size(var->type);
    size = (size < 8) ? 8 :size;
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

    offset = 16;

    printf("%s:\n", fn->name);

    printf("addi    sp,sp,-%d\n", stack_size);
    printf("sd      ra,%d(sp)\n", stack_size - 8);
    printf("sd      s0,%d(sp)\n", stack_size - 16);
    printf("addi    s0,sp,%d\n", stack_size);

    puts("");

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

    printf("ld      ra,%d(sp)\n", stack_size - 8);
    printf("ld      s0,%d(sp)\n", stack_size - 16);
    printf("addi    sp,sp,%d\n", stack_size);
    printf("jr      ra\n");
}

void codegen(Program *prog) {
    if (!table) {
        table = buildSymbolTable(prog);
        currentScope = table;
    }
    printf(".section	.rodata\n");
    for (int i = 0;i < prog->gvars->len;i++) {
        emit_data(prog->gvars->data[i]);
        puts("");
    }
    printf(".section	.text\n");
    puts("");
    for (int i = 0;i < prog->funcs->len;i++) {
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