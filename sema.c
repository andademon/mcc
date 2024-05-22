#include "include/mcc.h"

/**
 * a SymbolTable should save lexeme and it's scope;
 * a hashtable is an array of entrys, called buckets, indexed by an interger range from 0 to the table size minus one.
 * a hash function turns the search key (ID name) into an integer hash value.
 * 
 * next question: how to handle the scope information?
 * A: use seperate table for earch scope
 * 
*/

static SymbolTable *table;

SymbolTable *createSymbolTable(int scopeLevel, SymbolTable *parent) {
    SymbolTable *node = (SymbolTable *)malloc(sizeof(SymbolTable));
    node->scopeLevel = scopeLevel;
    node->entries = new_map();
    node->parent = parent;
    node->children = new_vec();
    return node;
}

void insert(SymbolTable *node, char *key, void *val) {
    map_put(node->entries, key, val);
}

void *lookup_current(SymbolTable *node, char *key) {
    return map_get(node->entries, key);
}

void *lookup(SymbolTable *node, char *key) {
    void *val = map_get(node->entries, key);
    if (val == NULL) {
        SymbolTable *temp = node->parent;
        while (temp) {
            val = map_get(temp->entries, key);
            if (val != NULL) break;
            temp = temp->parent;
        }
    }
    return val;
}

SymbolTable *enterScope(SymbolTable *parent) {
    int scopeLevel = parent->scopeLevel + 1;
    SymbolTable *node = createSymbolTable(scopeLevel, parent);
    vec_push(parent->children, node);
    return node;
}

SymbolTable *exitScope(SymbolTable *current) {
    return current->parent;
}

SymbolTable *buildSymbolTable(Program *prog) {
    SymbolTable *table = createSymbolTable(0, NULL);
    SymbolTable *currentScope = table;
    // 全局变量，scopeLevel为0
    for (int i = 0;i < prog->gvars->len;i++) {
        Var *var = prog->gvars->data[i];
        if (lookup_current(currentScope, var->name) != NULL) sema_error("duplicated symbol definition.");
        insert(currentScope, var->name, var);
    }
    for (int i = 0;i < prog->funcs->len;i++) {
        Function *fn = prog->funcs->data[i];
        if (lookup_current(currentScope, fn->name) != NULL) sema_error("duplicated symbol definition.");
        Var *func_var = new_var();
        func_var->name = fn->name;
        func_var->type = fn->node->type;

        insert(currentScope, fn->name, func_var);
        SymbolTable *temp_scope = enterScope(currentScope);
        // 函数形参，作为函数局部变量等同处理
        for (int i = 0;i < fn->params->len;i++) {
            Var *var = fn->params->data[i];
            if (lookup_current(temp_scope, var->name) != NULL) sema_error("duplicated symbol definition.");
            insert(temp_scope, var->name, var);
        }
        // 函数局部变量
        for (int i = 0;i < fn->lvars->len;i++) {
            Var *var = fn->lvars->data[i];
            if (lookup_current(temp_scope, var->name) != NULL) sema_error("duplicated symbol definition.");
            insert(temp_scope, var->name, var);
        }
        // 遍历stmt节点
        for(int i = 0;i < fn->stmts->len;i++) {
            walk_stmt((Node *)fn->stmts->data[i], temp_scope);
        }
    }
    return table;
}

/**
 * 遍历stmt节点，填充符号表
*/
void walk_stmt(Node *node, SymbolTable *table) {
    if (!node) return;
    SymbolTable *currentScope = table;
    switch (node->node_type) {
        case ND_BLOCK: {
            // if (node->decls && node->decls->len == 0) return;
            SymbolTable *temp_scope = enterScope(currentScope);
            // 给ND_BLOCK节点打上作用域信息
            node->scope = temp_scope;

            for (int i = 0;i < node->decls->len;i++) {
                Node *decl = node->decls->data[i];
                for (int i = 0;i < decl->declarators->len;i++) {
                    Node *declarator = decl->declarators->data[i];
                    Var *var = new_var();
                    var->name = declarator->name;
                    var->type = declarator->type;
                    var->init = declarator->init;
                    var->is_gval = false;
                    var->is_param = false;
                    if (lookup_current(temp_scope, var->name) != NULL) sema_error("duplicated symbol definition.");
                    insert(temp_scope, var->name, var);
                }
            }
            for (int i = 0;i < node->stmts->len;i++) {
                walk_stmt((Node *)node->stmts->data[i], temp_scope);
            }
            break;
        }
        case ND_LABEL_STMT: {
            walk_stmt(node->body, currentScope);
            break;
        }
        case ND_IF_STMT: {
            walk_stmt(node->then, currentScope);
            walk_stmt(node->els, currentScope);
            break;
        }
        case ND_FOR_STMT: {
            walk_stmt(node->body, currentScope);
            break;
        }
        case ND_DO_WHILE:
        case ND_WHILE_STMT: {
            walk_stmt(node->body, currentScope);
            break;
        }
        case ND_SWITCH_STMT: {
            for(int i = 0;i < node->cases->len;i++) {
                Node *case_ = node->cases->data[i];
                 walk_stmt(case_, currentScope);
            }
            break;
        }
        case ND_CASE: {
            walk_stmt(node->then, currentScope);
            break;
        }
    }
}

void sema_error(char *msg) {
    printf("semantic error: %s\n", msg);
    exit(1);
}

int compute_max_size(SymbolTable *table) {
    int count = 0;
    for (int i = 0;i < table->entries->keys->len;i++) {
        Var *var = table->entries->vals->data[i];
        count += var->type->size;
    }
    if (table->children->len == 0) return count;

    int max = 0;
    for (int i = 0;i < table->children->len;i++) {
        SymbolTable *temp = table->children->data[i];
        int max_size = compute_max_size(temp);
        if (max_size > max) max = max_size;
    }
    count += max;
    return count;
}

// 计算符号表中变量相对于栈底的偏移量
void compute_var_offset(SymbolTable *table, int base_offset) {
    if (table->scopeLevel > 0) {
        for (int i = 0;i < table->entries->keys->len;i++) {
            Var *var = table->entries->vals->data[i];
            var->offset = (base_offset -= var->type->size);
        }
    }

    for (int i = 0;i < table->children->len;i++) {
        SymbolTable *temp = table->children->data[i];
        compute_var_offset(temp, base_offset);
    }
}

// 给每个expression节点标注类型信息，和是否是左值
Type *type_expr(Node *node, SymbolTable *table) {
    if (!node) return NULL;
    switch (node->node_type) {
        case ND_UNARY_EXPR: {
            Type *ty = type_expr(node->body, table);
            switch (node->op_type) {
                case OP_NOT:
                case OP_PLUS:
                case OP_MINUS:
                case OP_INC:
                case OP_DEC: {
                    node->type = ty;
                    return ty;
                }
                case OP_ADDR: {
                    node->type = new_type(TY_POINTER_TO, 8, 8);
                    node->type->base = ty;
                    return node->type;
                }
                case OP_DEREF: {
                    if (ty->kind != TY_POINTER_TO) {
                        sema_error("not a pointer\n");
                    }
                    node->type = ty->base;
                    return node->type;
                }
                case OP_ARR_MEMBER: {
                    if (ty->kind != TY_ARRAY_OF && ty->kind != TY_POINTER_TO) {
                        sema_error("not an array or pointer\n");
                    }
                    node->type = ty->base;
                    return node->type;
                }
                default: 
                    sema_error("no such op type in unary expression");
            }
            break;
        }
        case ND_BINARY_EXPR: {
            Type *t1 = type_expr(node->lhs, table);
            Type *t2 = type_expr(node->rhs, table);
            if (!is_compatible(t1, t2)) {
                sema_error("incompatible type in binary expression\n");
            }
            node->type = t1;
            return node->type;
        }
        case ND_TERNARY_EXPR: {
            Type *t1 = type_expr(node->test, table);
            Type *t2 = type_expr(node->then, table);
            Type *t3 = type_expr(node->els, table);
            if (!is_compatible(t2, t3)) {
                sema_error("type mismatch in ternary expression\n");
            }
            node->type = t3;
            return node->type;
        }
        case ND_SEQUENCE_EXPR: {
            Type *temp;
            for (int i = 0;i < node->exprs->len;i++) {
                Node *expr = node->exprs->data[i];
                temp = type_expr(expr, table);
            }
            node->type = temp;
            return node->type;
        }
        case ND_CALLEXPR: {
            // 有三种调用函数情况：符号表中函数名、符号表中函数指针、不在符号表中的标识符
            Type *ty = type_expr(node->callee, table);
            if (
                (ty && ty->kind == TY_FUNC)
                || (ty && ty->base && ty->kind == TY_POINTER_TO && ty->base->kind == TY_FUNC)
                || (node->callee->node_type == ND_IDENT)
            );
            else {
                sema_error("callee in call expression is not a function");
            }
            if (ty) {
                if (ty && ty->base && ty->kind == TY_POINTER_TO && ty->base->kind == TY_FUNC) {
                    ty = ty->base;
                }
                // TODO: check args type
                if (ty->params->len != node->args->len) {
                    sema_error("function parameter count mismatch\n");
                }
                for (int i = 0;i < node->args->len;i++) {
                    Type *arg_type = type_expr(node->args->data[i], table);
                    Node *param = ty->params->data[i];
                    Type *param_type = param->type;

                    if (!is_compatible(arg_type, param_type)) {
                        sema_error("function parameter type mismatch\n");
                    }
                }
                node->type = ty->base;
                return node->type;
            }
            // 即使没有找到原函数，也要检查args的参数类型是合理类型
            else {
                for (int i = 0;i < node->args->len;i++) {
                    type_expr(node->args->data[i], table);
                }
            }
            return NULL;
        }
        case ND_IDENT: {
            Var *var = lookup(table, node->token->value);
            if (var) {
                // 如果id是一个函数名,将其视为指针,该指针指向函数
                if (var->type->kind == TY_FUNC) {
                    Type *ty = new_type(TY_POINTER_TO, 8, 8);
                    ty->base = var->type;
                    return ty;
                }
                // 考虑将数组名也视为指针
                // 如果id是数组名,将其视为指针,该指针指向数组的base元素
                if (var->type->kind == TY_ARRAY_OF) {
                    Type *ty = new_type(TY_POINTER_TO, 8, 8);
                    ty->base = var->type->base;
                    return ty;
                }
                return var->type;
            }
            else return NULL;
            // sema_error("undefined var");
            break;
        }
        case ND_NUM: {
            node->type = new_type(TY_INT, 4, 4);
            return node->type;
        }
        case ND_CHAR: {
            node->type = new_type(TY_CHAR, 1, 1);
            return node->type;
        }
    }
}

/**
 * 符号表在类型检查中起了一个非常重要的作用,那就是代表环境本身
*/

/**
 * 类型检查, 遍历检查stmt中的每一处expr的类型
 * table: 当前环境
*/
void type_stmt(Node *node, SymbolTable *table) {
    if (!node) return;
    switch(node->node_type) {
        case ND_BLOCK: {
            SymbolTable *currentScope = node->scope;
            for (int i = 0;i < node->decls->len;i++) {
                Node *decl = node->decls->data[i];
                type_expr(decl->init, currentScope);
            }
            for (int i = 0;i < node->stmts->len;i++) {
                type_stmt(node->stmts->data[i], currentScope);
            }
            break;
        }
        case ND_CASE: {
            type_stmt(node->then, table);
            break;
        }
        case ND_IF_STMT: {
            type_expr(node->test, table);
            type_stmt(node->then, table);
            type_stmt(node->els, table);
            break;
        }
        case ND_SWITCH_STMT: {
            for (int i = 0;i < node->cases->len;i++) {
                type_stmt(node->cases->data[i], table);
            }
            break;
        }
        case ND_DO_WHILE:
        case ND_WHILE_STMT: {
            type_expr(node->test, table);
            type_stmt(node->body, table);
            break;
        }
        case ND_FOR_STMT: {
            type_expr(node->init, table);
            type_expr(node->test, table);
            type_expr(node->update, table);
            type_stmt(node->body, table);
        }
        case ND_EXPR_STMT: {
            type_expr(node->body, table);
        }
        default:
            break;
    }
}

void type_decl(Node *node, SymbolTable *table) {
    if (!node) return;
    switch (node->node_type) {
        case ND_VAR_DECL: {

        }
    }
}

void check_type(Program *prog, SymbolTable *table) {
    for (int i = 0;i < prog->funcs->len;i++) {
        SymbolTable *currentScope = table->children->data[i];
        Function *fn = prog->funcs->data[i];
        Vector *decls = fn->node->body->decls;
        for (int i = 0;i < decls->len;i++) {
            Node *decl = decls->data[i];
            for (int j = 0;j < decl->declarators->len;j++) {
                Node *declarator = decl->declarators->data[j];
                type_expr(declarator->init, currentScope);
            }
        }
        for(int i = 0;i < fn->stmts->len;i++) {
            Node *stmt = fn->stmts->data[i];
            type_stmt(stmt, currentScope);
        }
    }
}

/**
 * 语义分析
 * 多遍遍历
 * 1.构造符号表(构造过程中检查重复定义)(函数类型由Function转换为Var, 也保存在符号表中)
 * 2.遍历ast，查找ident结点是否在符号表中已定义
 * 3.遍历ast，检查类型是否合规
*/
SymbolTable *sema(Program *prog) {
    table = buildSymbolTable(prog);

    compute_var_offset(table, -72);

    check_type(prog, table);

    puts("");

    return table;
}