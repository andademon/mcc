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

void *lookup(SymbolTable *node, char *key) {
    return map_get(node->entries, key);
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
    for (int i = 0;i < prog->gvars->len;i++) {
        Var *var = prog->gvars->data[i];
        if (lookup(currentScope, var->name) != NULL) sema_error("duplicated symbol definition.");
        insert(currentScope, var->name, var);
    }
    for (int i = 0;i < prog->funcs->len;i++) {
        Function *fn = prog->funcs->data[i];
        if (lookup(currentScope, fn->name) != NULL) sema_error("duplicated symbol definition.");
        insert(currentScope, fn->name, NULL);
        SymbolTable *temp_scope = enterScope(currentScope);
        for (int i = 0;i < fn->params->len;i++) {
            Var *var = fn->params->data[i];
            if (lookup(temp_scope, var->name) != NULL) sema_error("duplicated symbol definition.");
            insert(temp_scope, var->name, var);
        }
        for (int i = 0;i < fn->lvars->len;i++) {
            Var *var = fn->lvars->data[i];
            if (lookup(temp_scope, var->name) != NULL) sema_error("duplicated symbol definition.");
            insert(temp_scope, var->name, var);
        }
    }
    return table;
}

void sema_error(char *msg) {
    printf("semantic error: %s\n", msg);
    exit(1);
}

/**
 * 类型检查
*/
void check_type(Node *node) {
    switch(node->node_type) {
        case ND_BINARY_EXPR: {
            break;
        }
        default:
            break;
    }
}

void printSymbolTable(SymbolTable *table, int tabs) {
    printTab(tabs);
    printf("scopeLevel: %d\n", table->scopeLevel);
    for (int i = 0;i < table->entries->keys->len;i++) {
        printTab(tabs);
        printf("%s\n", table->entries->keys->data[i]);
    }
    for (int i = 0;i < table->children->len;i++) {
        printSymbolTable(table->children->data[i], tabs + 1);
    }
}

/**
 * 语义分析
 * 1.构造符号表(构造过程中检查重复定义)
 * 2.遍历ast，查找ident结点是否在符号表中已定义
 * 3.遍历ast，检查类型是否合规
*/

static SymbolTable *table;

void sema(Program *prog) {
    table = buildSymbolTable(prog);
    printSymbolTable(table, 0);
}