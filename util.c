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
    var->vals = new_vec();
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
    node->decl = NULL;
    node->stmt = NULL;
    node->expression = NULL;
    node->id = NULL;

    node->op = NULL;
    node->lhs = NULL;
    node->rhs = NULL;

    node->init = NULL;
    node->return_value = NULL;
    node->update = NULL;
    node->value = NULL;

    node->test = NULL;
    node->alternative = NULL;
    node->consequent = NULL;

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
            printTab(tabs + 1);
            for (int i = 0;i < node->exprs->len;i++) {
                printNode(node->exprs->data[i], tabs + 1);
            }
            break;
        case ND_BINARY_EXPR:
            printTab(tabs + 1);
            printf("left: \n");
            printNode(node->lhs, tabs + 2);
            printTab(tabs + 1);
            printf("right: \n");
            printNode(node->rhs, tabs + 2);
            printTab(tabs + 1);
            printf("op: %s\n", node->op->value);
            break;
        case ND_TERNARY_EXPR:
            printTab(tabs + 1);
            printf("test: \n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("consequent: \n");
            printNode(node->consequent, tabs + 2);
            printTab(tabs + 1);
            printf("alternative: \n");
            printNode(node->alternative, tabs + 2);
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
            printf("consequent: \n");
            printNode(node->consequent, tabs + 2);
            break;
        }
        case ND_CONTINUE_STMT:
            break;
        case ND_DECL_LIST:
            for (int i = 0;i < node->decls->len;i++) {
                printNode(node->decls->data[i], tabs + 1);
            }
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
        case ND_FUNCALL:
            printTab(tabs + 1);
            printf("function_name:\n");
            printTab(tabs + 2);
            printf("%s\n", node->id->value);
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
            printf("id: %s\n", node->id->value);
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
            printf("id: %s\n", node->id->value);
            break;
        case ND_IDENT:
            printTab(tabs + 1);
            printf("id: %s\n", node->id->value);
            break;
        case ND_IF_STMT:
            printTab(tabs + 1);
            printf("test: \n");
            printNode(node->test, tabs + 2);
            printTab(tabs + 1);
            printf("consequent: \n");
            printNode(node->consequent, tabs + 2);
            printTab(tabs + 1);
            printf("alternative: \n");
            printNode(node->alternative, tabs + 2);
            break;
        case ND_RETURN_STMT:
            printNode(node->body, tabs + 1);
            break;
        case ND_SWITCH_STMT:
            printTab(tabs + 1);
            printf("discriminant: \n");
            printNode(node->discriminant, tabs + 2);
            printTab(tabs + 1);
            printf("cases: \n");
            for (int i = 0;i < node->cases->len;i++) {
                printNode(node->cases->data[i], tabs + 2);
            }
            break;
        case ND_STMT_LIST:
            for (int i = 0;i < node->stmts->len;i++) {
                printNode(node->stmts->data[i], tabs + 1);
            }
            break;
        case ND_VAR_DECL:
            printTab(tabs + 1);
            printf("type: %d\n", node->decl_type);
            printTab(tabs + 1);
            printf("declarations: \n");
            for (int i = 0;i <= node->declarators->len;i++) {
                printNode(node->declarators->data[i], tabs + 2);
            }
            break;
        case ND_VAR_DECLARATOR:
            printTab(tabs + 1);
            printf("id: %s\n", node->id->value);
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
        case ND_NUM:
            printTab(tabs + 1);
            printf("value: %s\n", node->tok->value);
            break;
        case ND_STR:
            printTab(tabs + 1);
            printf("value: %s\n", node->tok->value);
            break;
        default:
            printNode(node->body, tabs + 1);
            break;
    }
}

void printVar(Node *node) {
    printf("");
    printf("type: %d\n", node->decl_type);
}

void printFunction(Node *node) {

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
}

// 将语法树结构转换为更易处理的var + function
Program *tree_to_prog(Program *prog) {
    Program *p = new_prog();
    for (int i = 0;i < prog->gvars->len;i++) {
        Node *decl = prog->gvars->data[i];
        for (int i = 0;i < decl->declarators->len;i++) {
            Node *declarator = decl->declarators->data[i];
            Var *var = new_var();
            var->name = declarator->id->value;
            var->type = declarator->decl_type;
            var->is_array = declarator->is_array;
            // if (var->is_array) var->len = declarator->len;
            var->init = declarator->init;
            var->offset = 0;

            vec_push(p->gvars, var);
        }
    }
    for (int i = 0;i < prog->funcs->len;i++) {
        Node *func = prog->funcs->data[i];

        Function *fn = new_func();
        fn->name = func->id->value;
        fn->node = func;
        // fn->lvars = func->body->decls;
        if (func->body->decls->len)
            for (int i = 0;i < func->body->decls->len;i++) {
                Node *decl = func->body->decls->data[i];
                for (int j = 0;j < decl->declarators->len;j++) {
                    Node *declarator = decl->declarators->data[j];
                    Var *var = new_var();
                    var->name = declarator->id->value;
                    var->type = declarator->decl_type;
                    var->is_array = declarator->is_array;
                    // if (var->is_array) var->len = declarator->len;
                    var->init = declarator->init;
                    var->offset = 0;
                    vec_push(fn->lvars, var);
                }
            }
        if (func->params->len)
            for (int i = 0;i < func->params->len;i++) {
                Node *param = func->params->data[i];
                Var *var = new_var();
                var->name = param->id->value;
                var->init = NULL;
                var->offset = 0;
                var->is_array = param->is_array;
                var->type = param->decl_type;
                vec_push(fn->params, var);
            }

        fn->stmts = func->body->stmts;
        vec_push(p->funcs, fn);
    }
    return p;
}
