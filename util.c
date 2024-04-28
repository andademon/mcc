#include "include/mcc.h"
#define KEYWORD_LEN 31
#define OPERATOR_LEN 23
#define BOUNDARYSIGN_LEN 11

/* C89关键字 */
static char* kw[] = {
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "int",
    "long",
    "register",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "unsigned",
    "union",
    "void",
    "volatile"
    "while",
};

/* 运算符 */
static char* op[] = {
    "+",
    "-",
    "*",
    "/",
    "%",
    "++",
    "--",
    "=",
    ">",
    "<",
    ">=",
    "<=",
    "==",
    "!=",
    "!",
    "&&",
    "||",
    ".",
    "?",
    ":",
    "&",
    "|",
    "\\",
};

/* 界符 */
static char* bound[] = {
    "{",
    "}",
    "(",
    ")",
    "[",
    "]",
    ",",
    ";",
    "#",
    "\'",
    "\"",
};

/* 是否是关键字 */
int isKeyWord(char *str)
{
    int i = 0;
    while (i < KEYWORD_LEN)
    {
        if (strcmp(str, kw[i]) == 0)
        {
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

/* 是否是运算符 */
int isOperator(char *str)
{
    int i = 0;
    while (i < OPERATOR_LEN)
    {
        if (strcmp(str, op[i]) == 0)
        {
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

/* 是否是界符 */
int isBoundarySign(char *str)
{
    int i = 0;
    while (i < BOUNDARYSIGN_LEN)
    {
        if (strcmp(str, bound[i]) == 0)
        {
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

/* 是否是字母 */
int isAlpha(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
    {
        return 1;
    }
    return 0;
}

/* 是否是数字 */
int isDigit(char c)
{
    if (c >= '0' && c <= '9')
    {
        return 1;
    }
    return 0;
}

/* 返回文件大小 */
long getFileSize(char *filename)
{
    FILE *fp;
    if ((fp = fopen(filename, "r")) == NULL)
    {
        printf("Error! Can't open file: %s", filename);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    return fileSize;
}

/* 读取文件，返回指向字符串的指针 */
char *readFile(char *filename)
{
    FILE* fp;
    if ((fp = fopen(filename, "r")) == NULL)
    {
        printf("Error! Can't open file: %s", filename);
        return NULL;
    }
    /* 获取文件大小 */
    long fileSize = getFileSize(filename);
    /* 读取文件 */
    char *buffer = (char *) malloc((fileSize + 1) * sizeof(char));
    fread(buffer, sizeof(char), fileSize, fp);
    fclose(fp);
    *(buffer + fileSize) = '\0';
    return buffer;
}

File *new_file(char *filename, char *path) {
    File *file = (File *)malloc(sizeof(File));
    file->filename = filename;
    file->path = path;
    return file;
}

Vector *new_vec() {
    Vector *v = malloc(sizeof(Vector));
    v->data = malloc(sizeof(void *) * 16);
    v->capacity = 16;
    v->len = 0;
    return v;
}

void vec_push(Vector *v, void *elem) {
    if (v->len == v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, sizeof(void *) * v->capacity);
    }
    v->data[v->len++] = elem;
}

void vec_pushi(Vector *v, int val) {
  vec_push(v, (void *)(intptr_t)val);
}

void *vec_pop(Vector *v) {
    return v->data[--v->len];
}

void *vec_last(Vector *v) {
    return v->data[v->len - 1];
}

bool vec_contains(Vector *v, void *elem) {
    for(int i = 0;i < v->len;i++)
        if (v->data[i] == elem) 
            return true;
    return false;
}

Map *new_map() {
    Map *map = (Map *)malloc(sizeof(Map));
    map->keys = new_vec();
    map->vals = new_vec();
    return map;
}

void map_put(Map *map, char *key, void *val) {
    vec_push(map->keys, key);
    vec_push(map->vals, val);
}

void map_puti(Map *map, char *key, int val) {
  map_put(map, key, (void *)(intptr_t)val);
}

void *map_get(Map *map, char *key) {
    for (int i = map->keys->len - 1;i >= 0;i--)
        if (!strcmp(map->keys->data[i], key))
            return map->vals->data[i];
    return NULL;
}

int map_geti(Map *map, char *key, int default_) {
  for (int i = map->keys->len - 1; i >= 0; i--)
    if (!strcmp(map->keys->data[i], key))
      return (intptr_t)map->vals->data[i];
  return default_;
}

Function *new_func() {
    Function *func = malloc(sizeof(Function));
    func->params = new_vec();
    func->lvars = new_vec();
    func->bbs = new_vec();
    return func;
}

Var *new_var() {
    Var *var = malloc(sizeof(Var));
    var->init = NULL;
    var->offset = 0;
    return var;
}

Program *new_prog() {
    Program *prog = malloc(sizeof(Program));
    prog->gvars = new_vec();
    prog->funcs = new_vec();
    return prog;
}

Node *new_node(char *type_name, int node_type) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->type_name = type_name;
    node->node_type = node_type;

    node->body = NULL;
    node->expression = NULL;

    node->token = NULL;
    node->lhs = NULL;
    node->rhs = NULL;

    node->init = NULL;
    node->update = NULL;

    node->test = NULL;
    node->els = NULL;
    node->then = NULL;

    node->decls = new_vec();
    node->stmts = new_vec();
    node->params = new_vec();
    node->args = new_vec();
    node->cases = new_vec();
    node->declarators = new_vec();
    node->exprs = new_vec();
    return node;
}

void printTab(int num) {
    if (num < 0) return;
    for(int i = 0;i < num;i++) {
        printf("    ");
    }
}

void printToken(Token *token) {
    printf("%d\t%d\t%d\t%s\n", token->id, token->line, token->type, token->value);
}

void printTokenList(Token *tokens) {
    printf("ID\tLINE\tTYPE\tVALUE\n");
    Token *current_token = tokens;
    while (current_token)
    {
        printToken(current_token);
        current_token = current_token->next;
    }
}

void printNode(Node *node, int tabs) {
    if (node == NULL) return;
    printTab(tabs);
    printf("%s\n", node->type_name); // 公共属性 type_name
    switch(node->node_type) {
        case ND_SEQUENCE_EXPR:
            for (int i = 0;i < node->exprs->len;i++) {
                printNode(node->exprs->data[i], tabs + 1);
            }
            break;
        case ND_ARR_EXPR:
            for (int i = 0;i < node->args->len;i++) {
                printNode(node->args->data[i], tabs + 1);
            }
            break;
        case ND_UNARY_EXPR:
            printTab(tabs + 1);
            printf("op: %s\n", node->token->value);
            printTab(tabs + 1);
            printf("is_prefix: %s\n", (node->is_prefix) ? "true" : "false");
            printNode(node->body, tabs + 2);
            break;
        case ND_BINARY_EXPR:
            printTab(tabs + 1);
            printf("left: \n");
            printNode(node->lhs, tabs + 2);
            printTab(tabs + 1);
            printf("right: \n");
            printNode(node->rhs, tabs + 2);
            printTab(tabs + 1);
            printf("op: %s\n", node->token->value);
            break;
        case ND_TERNARY_EXPR:
            printTab(tabs + 1);
            printf("test: \n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("then: \n");
            printNode(node->then, tabs + 2);
            printTab(tabs + 1);
            printf("els: \n");
            printNode(node->els, tabs + 2);
            break;
        case ND_BLOCK:
            for (int i = 0;i < node->decls->len;i++) {
                printNode(node->decls->data[i], tabs + 1);
            }
            for (int i = 0;i < node->stmts->len;i++) {
                printNode(node->stmts->data[i], tabs + 1);
            }
            break;
        case ND_BREAK_STMT:
            break;
        case ND_CASE: {
            printTab(tabs + 1);
            printf("test: \n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("then: \n");
            printNode(node->then, tabs + 2);
            break;
        }
        case ND_GOTO_STMT: {
            printTab(tabs + 1);
            printf("label: %s\n", node->token->value);
            break;
        }
        case ND_LABEL_STMT: {
            printTab(tabs + 1);
            printf("label: %s\n", node->token->value);
            printTab(tabs + 1);
            printf("body: \n");
            printNode(node->body, tabs + 2);
            break;
        }
        case ND_CONTINUE_STMT:
            break;
        case ND_EXPR_STMT:
            printNode(node->body, tabs + 1);
            break;
        case ND_FOR_STMT:
            printTab(tabs + 1);
            printf("init: \n");
            printNode(node->init, tabs + 2);
            printTab(tabs + 1);
            printf("test: \n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("update: \n");
            printNode(node->update, tabs + 2);
            printTab(tabs + 1);
            printf("body: \n");
            printNode(node->body, tabs + 2);
            break;
        case ND_CALLEXPR:
            // printTab(tabs + 1);
            // printf("function_name:\n");
            // printTab(tabs + 2);
            // if (node->token) printf("%s\n", node->token->value);
            printTab(tabs + 1);
            printf("callee:\n");
            printNode(node->callee, tabs + 2);
            if (node->args && node->args->len > 0) {
                printTab(tabs + 1);
                printf("args:\n");
                for (int i = 0; i < node->args->len; i++) {
                    printNode(node->args->data[i], tabs + 2);
                }
                
            }
            break;
        case ND_FUNC_DECL:
            printTab(tabs + 1);
            printf("id: %s\n", node->name);
            printTab(tabs + 1);
            printf("params: \n");
            for (int i = 0;i < node->params->len;i++) {
                printNode(node->params->data[i], tabs + 2);
            }
            printTab(tabs + 1);
            printf("body: \n");
            printNode(node->body, tabs + 2);
            break;
        case ND_FUNC_PARAM:
            printTab(tabs + 1);
            printf("id: %s\n", node->name);
            break;
        case ND_IF_STMT:
            printTab(tabs + 1);
            printf("test: \n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("then: \n");
            printNode(node->then, tabs + 2);
            printTab(tabs + 1);
            printf("els: \n");
            printNode(node->els, tabs + 2);
            break;
        case ND_RETURN_STMT:
            printNode(node->body, tabs + 1);
            break;
        case ND_SWITCH_STMT:
            printTab(tabs + 1);
            printf("test: \n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("cases: \n");
            for (int i = 0;i < node->cases->len;i++) {
                printNode(node->cases->data[i], tabs + 2);
            }
            break;
        case ND_VAR_DECL:
            printTab(tabs + 1);
            printf("declarations: \n");
            for (int i = 0;i <= node->declarators->len;i++) {
                printNode(node->declarators->data[i], tabs + 2);
            }
            break;
        case ND_VAR_DECLARATOR:
            printTab(tabs + 1);
            printf("id: %s\n", node->name);
            printTab(tabs + 1);
            printf("init: \n");
            printNode(node->init, tabs + 2);
            break;
        case ND_WHILE_STMT:
            printTab(tabs + 1);
            printf("test:\n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("body:\n");
            printNode(node->body, tabs + 2);
            break;
        case ND_DO_WHILE:
            printTab(tabs + 1);
            printf("body:\n");
            printNode(node->body, tabs + 2);
            printTab(tabs + 1);
            printf("test:\n");
            printNode(node->test, tabs + 2);
            break;
        case ND_IDENT:
            printTab(tabs + 1);
            printf("id: %s\n", node->token->value);
            break;
        case ND_NUM:
            printTab(tabs + 1);
            printf("value: %s\n", node->token->value);
            break;
        case ND_STR:
            printTab(tabs + 1);
            printf("value: %s\n", node->token->value);
            break;
        default:
            printNode(node->body, tabs + 1);
            break;
    }
}

void printProgram(Program *prog) {
    if (prog && prog->gvars && prog->gvars->len) {
        printf("Global Variables:\n");
        for (int i = 0;i < prog->gvars->len;i++)
            printNode(prog->gvars->data[i], 0);
    }
    if (prog && prog->funcs && prog->funcs->len) {
        printf("Functions:\n");
        for (int i = 0;i < prog->funcs->len;i++)
            printNode(prog->funcs->data[i], 0);
    }
    puts("");
}

int str_to_int(char *str) {
    int len = strlen(str);
    int count = 0;
    int temp = 1;
    for (int i = len - 1;i >= 0;i--) {
        count += (temp * ((int)str[i] - 48));
        temp *= 10;
    }
    return count;
}

// int get_type_size(int type) {
//     switch (type) {
//         case VOID:
//         case CHAR:
//             return 1;
//         case INT:
//             return 4;
//         default:
//             return 0;
//     }
// }

char *get_size_name(int size) {
    switch (size) {
        case 1:
            return ".byte";
        case 4:
            return ".word";
        case 8:
            return ".dword";
        default:
            printf("unknown size: %d\n", size);
            exit(1);
    }
}

// int compute_var_memory_size(Var *var) {
//     int type_size = get_type_size(var->type);
//     if (var->is_pointer) return 8;
//     if (var->is_array && var->is_param) return 8;
//     if (var->is_array) return type_size * var->len;
//     return type_size;
// }

// 将语法树结构转换为更易处理的var + function
Program *tree_to_prog(Program *prog) {
    Program *p = new_prog();
    for (int i = 0;i < prog->gvars->len;i++) {
        Node *decl = prog->gvars->data[i];
        for (int i = 0;i < decl->declarators->len;i++) {
            Node *declarator = decl->declarators->data[i];
            Var *var = new_var();
            var->name = declarator->name;
            var->type = declarator->type;
            // var->type = declarator->base_type;
            // var->is_array = declarator->is_array;
            // var->is_pointer = declarator->is_pointer;
            var->init = declarator->init;
            var->is_gval = true;
            var->is_param = false;
            // if (var->is_array) {
            //     if (var->init) {
            //         var->len = var->init->len;
            //     }
            //     else var->len = declarator->len;
            // }
            // else {
            //     var->len = 1;
            // }
            // if (var->is_pointer) {
            //     var->type_size = 8;
            // }
            // else {
            //     var->type_size = (get_type_size(var->type) > 4) ? get_type_size(var->type) : 4;
            // }
            // var->type_size = get_type_size(var->type);
            // var->memory_size = compute_var_memory_size(var);
            vec_push(p->gvars, var);
        }
    }
    for (int i = 0;i < prog->funcs->len;i++) {
        Node *func = prog->funcs->data[i];

        Function *fn = new_func();
        fn->name = func->name;
        fn->node = func;
        // fn->lvars = func->body->decls;
        if (func->body->decls->len)
            for (int i = 0;i < func->body->decls->len;i++) {
                Node *decl = func->body->decls->data[i];
                for (int j = 0;j < decl->declarators->len;j++) {
                    Node *declarator = decl->declarators->data[j];
                    Var *var = new_var();
                    var->name = declarator->name;
                    var->type = declarator->type;
                    // var->type = declarator->base_type;
                    // var->is_array = declarator->is_array;
                    // var->is_pointer = declarator->is_pointer;
                    var->init = declarator->init;
                    var->is_gval = false;
                    var->is_param = false;
                    // if (var->is_array) {
                    //     if (var->init) {
                    //         var->len = var->init->len;
                    //     }
                    //     else var->len = declarator->len;
                    // }
                    // else {
                    //     var->len = 1;
                    // }
                    // var->type_size = get_type_size(var->type);
                    // var->memory_size = compute_var_memory_size(var);
                    vec_push(fn->lvars, var);
                }
            }
        if (func->params->len)
            for (int i = 0;i < func->params->len;i++) {
                Node *param = func->params->data[i];
                Var *var = new_var();
                var->type = param->type;
                var->name = param->type->name;
                // var->name = param->id->value;
                // var->is_array = param->is_array;
                // var->is_pointer = param->is_pointer;
                // var->derived_type = param->type;
                // var->type = param->base_type;
                var->is_gval = false;
                var->is_param = true;
                // if (var->is_pointer || var->is_array) {
                //     var->type_size = 8;
                // }
                // else {
                //     var->type_size = (get_type_size(var->type) > 4) ? get_type_size(var->type) : 4;
                // }
                // var->type_size = get_type_size(var->type);
                // var->memory_size = compute_var_memory_size(var);
                vec_push(fn->params, var);
            }

        fn->stmts = func->body->stmts;
        vec_push(p->funcs, fn);
    }
    return p;
}

// char *get_base_type_name(int type) {
//     switch (type) {
//         case VOID: return "void";
//         case CHAR: return "char";
//         case INT: return "int";
//         default:
//             printf("unknown base type: %s\n", type);
//             exit(1);
//     }
// }

void printSymbolTable(SymbolTable *table, int tabs) {
    printTab(tabs);
    printf("scopeLevel: %d\n", table->scopeLevel);
    for (int i = 0;i < table->entries->keys->len;i++) {
        printTab(tabs);
        Var *var = table->entries->vals->data[i];
        if (!var) continue;
        printf("%s\n", var->name);
        {
            printType(var->type, tabs + 1);
            printTab(tabs + 1);
            if (var->type->kind != TY_FUNC)
                printf("offset: %d\n", var->offset);
            // printFullType(var->derived_type, tabs + 1);
            // printTab(tabs + 1);
            // printFullType(var->derived_type);
            // printf("------\n");
            // printf("base_type: %s\n", get_base_type_name(var->type));
            // printTab(tabs + 1);
            // printf("is_param: %s\n", (var->is_param) ? "true" : "false");
            // printTab(tabs + 1);
            // printf("is_array: %s\n", (var->is_array) ? "true" : "false");
            // if (var->is_array) {
            //     printTab(tabs + 1);
            //     printf("array_len: %d\n", var->len);
            // }
            // printTab(tabs + 1);
            // printf("is_pointer: %s\n", (var->is_pointer) ? "true" : "false");
            // printTab(tabs + 1);
            // printf("type_size: %d\n", var->type_size);
            // printTab(tabs + 1);
            // printf("memory_size: %d\n", var->memory_size);
        }        
    }
    for (int i = 0;i < table->children->len;i++) {
        printSymbolTable(table->children->data[i], tabs + 1);
    }
}

char *get_access_unit(int size) {
    switch (size) {
        case 1: return "b";
        case 4: return "w";
        case 8: return "d";
        default:
            printf("unknown access unit size: %d\n", size);
            exit(1);
    }
}

Type *new_type(int kind, int size, int align) {
    Type *ty = (Type *)malloc(sizeof(Type));
    ty->kind = kind;
    ty->size = size;
    ty->align = align;
    return ty;
}

bool is_compatible(Type *t1, Type *t2) {
    if (t1->kind != t2->kind) {
        if (t1->kind == TY_ARRAY_OF && t2->kind == TY_POINTER_TO
            || t1->kind == TY_POINTER_TO && t2->kind == TY_ARRAY_OF);
        else return false;
    }
    switch (t1->kind) {
        case TY_VOID:
        case TY_CHAR:
        case TY_INT:
            return t1->kind == t2->kind;
        case TY_ARRAY_OF:
        case TY_POINTER_TO: {
            return is_compatible(t1->base, t2->base);
        }
        // case TY_ARRAY_OF: {
        //     return t1->array_len == t2->array_len 
        //             && t1->align == t2->align 
        //             && is_compatible(t1->base, t2->base);
        // }
        case TY_FUNC: {
            if (t1->params->len != t2->params->len) return false;
            for (int i = 0;i < t1->params->len;i++) {
                Node *n1 = t1->params->data[i];
                Node *n2 = t2->params->data[i];
                if (!is_compatible(n1->type, n2->type)) return false;
            }
            return is_compatible(t1->base, t2->base);
        }
    }
}

Type *get_base(Type *type) {
    switch(type->kind) {
        case TY_FUNC:
        case TY_POINTER_TO:
        case TY_ARRAY_OF: {
            if (type->base) {
                return get_base(type->base);
            }
            else return type;
        }
        default: {
            printf("Unknown type: ");
            exit(1);
        }
    }
}

void printType(Type *type, int tabs) {
    switch (type->kind) {
        case TY_VOID: {
            printTab(tabs);
            printf("void\n");
            printTab(tabs + 1);
            printf("size: %d\n", type->size);
            break;
        }
        case TY_INT: {
            printTab(tabs);
            printf("int\n");
            printTab(tabs + 1);
            printf("size: %d\n", type->size);
            break;
        }
        case TY_CHAR: {
            printTab(tabs);
            printf("char\n");
            printTab(tabs + 1);
            printf("size: %d\n", type->size);
            break;
        }
        case TY_POINTER_TO: {
            printTab(tabs);
            printf("pointer\n");
            printTab(tabs + 1);
            printf("size: %d\n", type->size);
            break;
        }
        case TY_ARRAY_OF: {
            printTab(tabs);
            printf("array\n");
            printTab(tabs + 1);
            printf("len: %d\n", type->array_len);
            printTab(tabs + 1);
            printf("align: %d\n", type->align);
            printTab(tabs + 1);
            printf("size: %d\n", type->size);
            break;
        }
        case TY_FUNC: {
            printTab(tabs);
            printf("function\n");
            break;
        }
    }
}

void printFullType(Type *type, int tabs) {
    switch (type->kind) {
        case TY_VOID:
            printTab(tabs);
            printf("void\n");
            printf("\t%d\t%d\n", type->size, type->align);
            break;
        case TY_CHAR:
            printTab(tabs);
            printf("char\n");
            printf("\t%d\t%d\n", type->size, type->align);
            break;
        case TY_INT:
            printTab(tabs);
            printf("int\n");
            printf("\t%d\t%d\n", type->size, type->align);
            break;
        case TY_POINTER_TO:
            printTab(tabs);
            printf("pointer to\n");
            printf("\t%d\t%d\n", type->size, type->align);
            printFullType(type->base, tabs + 1);
            break;
        case TY_ARRAY_OF:
            printf("array of\n");
            printf("\t%d\t%d\t%d\n", type->size, type->align, type->array_len);
            printFullType(type->base, tabs + 1);
            break;
        case TY_FUNC:
            printf("function\n");
            printf("params:\n");
            for (int i = 0;i < type->params->len;i++) {
                Node *param = type->params->data[i];
                printFullType(param->type, tabs + 1);
            }
            printf("return type:\n");
            printFullType(type->base, tabs + 1);
            break;
        default:
            printf("Unknown type!\n");
            exit(1);
    }
}

// int get_type_size(Type *type) {
//     switch (type->kind) {
//         case TY_VOID: return 0;
//         case TY_CHAR: return 1;
//         case TY_INT: return 4;
//         case TY_POINTER_TO: return 8;
//         default: return 0;
//     }
// }

void complete_type_size(Type *type) {
    switch (type->kind) {
        case TY_VOID:
        case TY_CHAR:
        case TY_INT:
            return;
        case TY_POINTER_TO: {
            complete_type_size(type->base);
            type->align = 8;
            type->size = 8;
            break;
        };
        case TY_FUNC: {
            complete_type_size(type->base);
            for (int i = 0;i < type->params->len;i++) {
                Node *node = type->params->data[i];
                complete_type_size(node->type);
            }
            type->size = 0;
            type->align = 0;
            break;
        };
        case TY_ARRAY_OF: {
            complete_type_size(type->base);
            type->align = type->base->size;
            type->size = type->array_len * type->align;
            break;
        }
    }
}

// 判断一个节点是否为左值
bool is_lval(Node *node) {

}