#include <stdio.h>
#include <stdlib.h>
#include "include/mcc.h"

void emit_data(Var *var);
void emit_code(Function *fn);
static void emit(char *op, Reg *r0, Reg *r1, Reg *r2);
static void emit2(char *op, Reg *r0, Reg *r2);

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

static int offset = 0;

static SymbolTable *table;

static SymbolTable *currentScope;

static Vector *irs;
static BB *out;

Map *op_map;

static int new_label() {
    return nlabel++;
}

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
    Reg *r0 = new_reg();
    Reg *r1 = gen_expr(node->lhs);
    Reg *r2 = gen_expr(node->rhs);
    emit(node->op->value, r0, r1, r2);
    return r0;
}

// 生成expr IR
static Reg *gen_expr(Node *node) {
    // if (!node) return NULL;
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
            printf("mv r%d,r%d\n", r0->vn, r1->vn);
            break;
        }
        case ND_FUNCALL: {
            Reg *args[6];
            // 对于函数参数的处理，如果是立即数，直接li加载，否则（是左值）看arg在哪里保存
            for (int i = 0;i < node->args->len;i++) {
                args[i] = gen_expr(node->args->data[i]);
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

char *to_str(char *op) {
    map_put(op_map, "+", "add");
    map_put(op_map, "-", "sub");
    map_put(op_map, "*", "mul");
    map_put(op_map, "/", "div");
    // map_put(op_map, ">", "slt");
    // map_put(op_map, "<", "slt");
    // map_put(op_map, ">=", "");
    // map_put(op_map, "<=", "");
    // map_put(op_map, "==", "");
    // map_put(op_map, "!=", "");
    char* rs = map_get(op_map, op);
    if (rs == NULL) return op;
    return rs;
}

static void emit(char *op, Reg *r0, Reg *r1, Reg *r2) {
    printf("%s r%d,r%d,r%d\n", to_str(op), r0->vn, r1->vn, r2->vn);
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

// 分支IR,绑定真分支与跳转分支
static IR *br(Reg *r, BB *then, BB *els) {
    printf("beq r%d,zero,.L%d\n", r->vn, els->label);
    printf("j %d\n", then->label);
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
    ir->bb1 = bb;
    return ir;
}

// 对于每一个basic block,确认好它的输入输出（如：有几个输入？几个输出？输入和输出在基本块的什么位置？）
// 生成中间代码的第一步，好像还用不到basic block,basic block的主要作用是在后续优化
// 那现在离开basic block怎么生成代码？
// 在该生成basic block的地方顺序打印ir,生成basic block前先生成该块的前置块和后置块，然后basic block从入口到出口顺序执行
static void gen_stmt(Node *node) {
    switch (node->node_type) {
        case ND_BLOCK: {
            BB *body = new_bb();
            for (int i = 0;i < node->stmts->len;i++) {
                gen_stmt(node->stmts->data[i]);
            }
            break;
        }
        case ND_WHILE_STMT: {
            BB *test = new_bb();
            BB *body = new_bb(); // 
            BB *break_ = new_bb(); // while_stmt break后的去处
            br(gen_expr(node->test), body, break_);
            gen_expr(node->body);
            // br(gen_expr())
            break;
        }
        case ND_FOR_STMT: {
            BB *body = new_bb();
            BB *test = new_bb();
            BB *break_ = new_bb();
            if (node->init) 
                gen_stmt(node->init);
            jmp(test);
            out = test;
            if (node->test) {
                Reg *r = gen_expr(node->test);
                br(r, body, break_);
            } else {
                jmp(body);
            }

            out = body;
            gen_stmt(node->body);

            if (node->update)
                gen_expr(node->update);
            gen_expr(node->update);
            
            jmp(test);
            break;
        }
        case ND_IF_STMT: {
            BB *then = new_bb();
            BB *els = new_bb();
            BB *last = new_bb();

            br(gen_expr(node->test), then, els);

            out = then;
            gen_stmt(node->consequent);

            jmp(last);

            out = els;
            if (node->alternative)
                gen_stmt(node->alternative);
            jmp(last);

            out = last;
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
    Reg *r0 = new_reg();
    var->offset = offset;
    Var *temp_var = lookup(currentScope, var->name);
    temp_var->offset = offset;
    printf("mv r%d,a%d\n", r0->vn, i);
    printf("sw r%d,-%d(s0)\n", r0->vn, offset);
    offset += 8;
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

    offset = 0;

    printf(".%s\n", fn->name);

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
    if (!irs) irs = new_vec();
    if (!table) {
        table = buildSymbolTable(prog);
        currentScope = table;
    }
    if (!op_map) {
        op_map = new_map();
        map_put(op_map, "+", "add");
        map_put(op_map, "-", "sub");
        map_put(op_map, "*", "mul");
        map_put(op_map, "/", "div");
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
        printf(".globl	%s\n", fn->name);
        printf(".type	%s, @function\n", fn->name);
        currentScope = currentScope->children->data[i];
        emit_code(fn);
        puts("");
        currentScope = exitScope(currentScope);
    }
}

int compute_offset(Var *var) {

}