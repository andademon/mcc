#ifndef __MCC_H__
#define __MCC_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#define TRUE 1
#define FALSE 0
#define EOF (-1)

typedef struct File {
    char *filename;
    char *path;
    char *content;
} File;

File *new_file(char *filename, char *path);

/* 工具类 */
typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vec(void);
void vec_push(Vector *v, void *elem);
void vec_pushi(Vector *v, int val);
void *vec_pop(Vector *v);
void *vec_last(Vector *v);
bool vec_contains(Vector *v, void *elem);

typedef struct {
    Vector *keys;
    Vector *vals;
} Map;

Map *new_map(void);
void map_put(Map *map, char *key, void *val);
void map_puti(Map *map, char *key, int val);
void *map_get(Map *map, char *key);
int map_geti(Map *map, char *key, int default_);

/* (Alpha | _)(Alpha | Digit | _)* */
/* (Digit)(Digit)* */
/* "\"(* except for \")\"" */
typedef enum
{
    KEYWORD, /* 关键字 */
    IDENTIFIER, /* 标识符 */
    NUMBER, /* 数字常量 */
    CHARACTER, /* 字符常量 */
    STRING, /* 字符串常量 */
    OPERATOR, /* 运算符 */
    BOUNDARYSIGN, /* 界符 */
    COMMENT, /* 注释 */
    OTHER, /* 其他 */
    TK_EOF
} TOKEN_TYPE;\

/* ---util.c--- */
long getFileSize(char *filename);
char *readFile(char *filename);

int isKeyWord(char *str);
int isOperator(char *str);
int isBoundarySign(char *str);
int isAlpha(char c);
int isDigit(char c);

/* ---scanner.c--- */

/* Token */
typedef struct Token
{
    int id;
    TOKEN_TYPE type;
    char *value;
    int line;
    struct Token *next;
} Token;

static Token *new_token(int id, int line, TOKEN_TYPE type, char *value);
Token *Lexer(char *str);

char *new_str();
char *str_push(char *str, char c);
char *str_pop(char *str);


/* ---parser.c--- */

// AST node type
enum {
    ND_NULL_EXPR, // Do nothing

    ND_STMT_LIST, // stmts
    ND_DECL_LIST, // decls

    ND_BLOCK,     // { ... }
    ND_CASE,      // case
    ND_GOTO_STMT,      // "goto"
    ND_CONTINUE_STMT, // continue
    ND_BREAK_STMT, // break
    ND_RETURN_STMT,    // return
  
    ND_VAR_DECL, // Variable declaration
    ND_TYPE_DECL, // type specifier
    ND_VAR_DECLARATOR,

    ND_FUNC_DECL, // Function decl
    ND_FUNCALL,   // Function call
    ND_FUNC_PARAM,

    ND_IF_STMT,
    ND_SWITCH_STMT,
    ND_WHILE_STMT,
    ND_DO_WHILE,
    ND_FOR_STMT,
    ND_JUMP_STMT,
    ND_EXPR_STMT, // Expression statement

    ND_UNARY_EXPR, // unary expression
    ND_BINARY_EXPR, // binary expression
    ND_TERNARY_EXPR, // ternary expression
    ND_SEQUENCE_EXPR, // sequence expression

    ND_NUM,       // literal Number
    ND_CHAR,
    ND_STR,
    ND_IDENT,
    ND_ARR_INIT,  // array init
    ND_CAST,      // Type cast
    ND_PROGRAM
};

enum {
    VOID = 0,
    CHAR,
    INT,
    STRUCT
};

enum {
    OP_ADD = 0,   // +
    OP_SUB,       // -
    OP_MUL,       // *
    OP_DIV,       // /
    OP_PLUS,      // unary +
    OP_MINUS,     // unary -
    OP_INC,       // increment ++
    OP_DEC,       // decrement --
    OP_MOD,       // %
    OP_BITAND,    // &
    OP_BITOR,     // |
    OP_BITXOR,    // ^
    OP_SHL,       // <<
    OP_SHR,       // >>
    OP_EQ,        // ==
    OP_NE,        // !=
    OP_LT,        // <
    OP_LE,        // <=
    OP_GT,       // >
    OP_GE,       // >=
    OP_ASSIGN,    // =
    OP_COND,      // ?:
    OP_COMMA,     // ,
    OP_MEMBER,    // . (struct member access)
    OP_ARR_MEMBER,  // arr [ expr ] array member access
    OP_ADDR,      // unary &
    OP_DEREF,     // unary *
    OP_NOT,       // !
    OP_BITNOT,    // ~
    OP_LOGAND,    // &&
    OP_LOGOR,     // ||
};

typedef struct Node {
    char *type_name;
    int node_type;
    char *value;

    Token *tok;

    // operator
    int op_type;

    // type decl
    int decl_type;
    bool is_array;
    bool is_pointer;
    int len;

    // func decl
    struct Node *decl;
    struct Node *stmt;

    // body
    struct Node *body;

    // node list
    struct Node *next;

    // VariableDeclaration
    // struct Node *declarations;

    // ExprStmt / array_name [ expression ] in UnaryExpression
    struct Node *expression;

    // BinaryExpr
    struct Node *lhs;
    struct Node *rhs;
    // OPERATOR op;
    Token *op;
    bool is_prefix; // for unary-expr && postfix-expr op
    // Function
    // struct Node *params;
    Token *id;

    // if ( test ) consequent else alternative
    // for ( init ; test ; update) body
    // while ( test ) body
    // switch ( discriminant ) body
    // case var: body
    struct Node *test;
    struct Node *consequent; // case also has consequent
    struct Node *alternative;
    struct Node *init;
    struct Node *update;
    struct Node *discriminant;
    struct Node *return_value;

    Vector *decls;
    Vector *stmts;
    Vector *cases; // switch-case
    Vector *params; // function decl
    Vector *args; // function call, array init
    Vector *declarators;

    Vector *exprs; // sequence expression
} Node;

Node *new_node(char *type_name, int node_type);

// typedef struct {
//     NODE_TYPE type;
//     Node *exprs;
// } SequenceExpr;

typedef struct {
  Vector *gvars;
  Vector *funcs;
} Program;

Program *new_prog();

typedef struct {
  char *name;
  Node *node;
  Vector *params;
  Vector *lvars;
  Vector *stmts;
  Vector *bbs;
} Function;

Function *new_func();

typedef struct {
    char *name;
    int type;
    char *data;

    int offset; // offset from fp pointer
    int size; // type size in memory
    int memory; // memory size
    bool is_array;
    bool is_pointer;
    bool is_gval;

    // if is array
    int len;
    Vector *vals;
    
    Node *init;
} Var;

Var *new_var();

Program *parse(Token *tokens);

// sema.c

/* 符号表 */
typedef struct SymbolTable {
    int scopeLevel;
    Map *entries;
    struct SymbolTable *parent;
    Vector *children;
} SymbolTable;

struct SymbolTable *createSymbolTable(int scopeLevel, struct SymbolTable *parent);
struct SymbolTable *enterScope(struct SymbolTable *parent);
struct SymbolTable *exitScope(struct SymbolTable *current);
void insert(struct SymbolTable *node, char *key, void *val);
void *lookup(struct SymbolTable *node, char *name);
void sema_error(char *msg);
struct SymbolTable *buildSymbolTable(Program *prog);
void sema(Program *prog);
void printSymbolTable(SymbolTable *table, int tabs);

// codegen.c

void codegen(Program *prog);

/* IR类型 */

enum {
    IR_ADD = 1,
    IR_ADDI,
    IR_SUB,
    IR_LUI,
    IR_AUIPC,

    IR_NEG,
    IR_MV,
    IR_NOP,
    IR_LI,

    IR_XOR,
    IR_OR,
    IR_AND,
    IR_XORI,
    IR_ORI,
    IR_ANDI,

    IR_RET,
    IR_J,
    IR_JR,
};

/* 寄存器 */

typedef struct {
    int vn; // virtual register number
    int rn; // real register number

    bool using;
} Reg;

/* 基本块 */
typedef struct {
    int label;
    Vector *ir;

    Vector *in_regs;
    Vector *out_regs;
} BB; /*basic block*/

/* 三地址代码形式的IR */
typedef struct {
    int op;
    Reg *r0;
    Reg *r1;
    Reg *r2;

    int ir_type;

    BB *bb1;
    BB *bb2;

    // Load/store size in bytes
    int size;
} IR;

void printTab(int num);
void printToken(Token *token);
void printTokenList(Token *tokens);
void printNode(Node *node, int tabs);
void printVar(Node *node);
void printProgram(Program *prog);

// Reg *new_reg();

// 将语法树结构转换为更易处理的var + function
Program *tree_to_prog(Program *prog);
int str_to_int(char *str);
int get_size(int type);
char *get_size_name(int size);
int compute_var_size(Var *var);

#endif