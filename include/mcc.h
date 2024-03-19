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

// AST node
typedef enum {
  ND_NULL_EXPR, // Do nothing
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_NEG,       // unary -
  ND_MOD,       // %
  ND_BITAND,    // &
  ND_BITOR,     // |
  ND_BITXOR,    // ^
  ND_SHL,       // <<
  ND_SHR,       // >>
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_COND,      // ?:
  ND_COMMA,     // ,
  ND_MEMBER,    // . (struct member access)
  ND_ADDR,      // unary &
  ND_DEREF,     // unary *
  ND_NOT,       // !
  ND_BITNOT,    // ~
  ND_LOGAND,    // &&
  ND_LOGOR,     // ||
  ND_RETURN_STMT,    // "return"
  ND_IF,        // "if"
  ND_FOR,       // "for"
  ND_WHILE,     // "while"
  ND_DO,        // "do"
  ND_SWITCH,    // "switch"
  ND_CASE,      // "case"
  ND_BLOCK,     // { ... }
  ND_GOTO_STMT,      // "goto"
  ND_GOTO_EXPR, // "goto" labels-as-values
  ND_BREAK_STMT,
  ND_CONTINUE_STMT,
  ND_LABEL,     // Labeled statement
  ND_LABEL_VAL, // [GNU] Labels-as-values
  ND_FUNCALL,   // Function call
  ND_FUNC_PARAM,
  ND_STMT_LIST,
  ND_DECL_LIST,
  ND_VAR_DECLARATOR,

  ND_IF_STMT,
  ND_SWITCH_STMT,
  ND_WHILE_STMT,
  ND_FOR_STMT,
  ND_JUMP_STMT,

  ND_EXPR_STMT, // Expression statement
  ND_TYPE_DECL, // type specifier
  ND_BINARY_EXPR, // binary expression
  ND_ASSIGN_EXPR, // assignment expression
  ND_VAR,       // Variable
  ND_VAR_DECL, // Variable declaration
  ND_VLA_PTR,   // VLA designator
  ND_FUNC_DECL,
  ND_NUM,       // literal Number
  ND_CHAR,
  ND_IDENT,
  ND_CAST,      // Type cast
  ND_MEMZERO,   // Zero-clear a stack variable
  ND_ASM,       // "asm"
  ND_CAS,       // Atomic compare-and-swap
  ND_EXCH,      // Atomic exchange
  ND_PROGRAM
} NODE_TYPE;

typedef enum {
    VOID = 0,
    CHAR,
    INT,
    FLOAT,
    DOUBLE,
    SHORT,
    LONG,
    STRUCT
} TYPE;

typedef enum {
    OP_ADD,       // +
    OP_SUB,       // -
    OP_MUL,       // *
    OP_DIV,       // /
    OP_NEG,       // unary -
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
    OP_ASSIGN,    // =
    OP_COND,      // ?:
    OP_COMMA,     // ,
    OP_MEMBER,    // . (struct member access)
    OP_ADDR,      // unary &
    OP_DEREF,     // unary *
    OP_NOT,       // !
    OP_BITNOT,    // ~
    OP_LOGAND,    // &&
    OP_LOGOR,     // ||
} OPERATOR_TYPE;

typedef struct Node {
    char *type_name;
    NODE_TYPE node_type;
    char *value;

    Token *tok;
    // type decl
    TYPE decl_type;
    bool is_array;

    // func decl
    struct Node *decl;
    struct Node *stmt;

    // body
    struct Node *body;

    // node list
    struct Node *next;

    // VariableDeclaration
    // struct Node *declarations;

    // ExprStmt
    struct Node *expression;

    // BinaryExpr
    struct Node *lhs;
    struct Node *rhs;
    // OPERATOR op;
    Token *op;
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
    Vector *cases;
    Vector *params;
    Vector *args;
    Vector *declarators;
} Node;

Node *new_node(char *type_name, NODE_TYPE node_type);

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
  Vector *lvars;
  Vector *stmts;
  Vector *bbs;
} Function;

Function *new_func();

typedef struct {
    char *name;
    Node *node;
    int type;

    void *val;

    bool is_global;
    bool is_array;
    int len; // if is array
    Vector *vals;
    // bool is_struct;
} Var;

Var *new_var();

Program *parse(Token *tokens);

// sema.c

// codegen.c

// void codegen(Program *prog);
void codegen(Node *node);

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
};

/* 三地址代码形式的IR */
typedef struct {
    int op;
    int r0;
    int r1;
    int r2;

    int ir_type;
} IR;

/* 基本块 */
typedef struct {
    int label;
    IR *ir;
    int ir_num;
} BB; /*basic block*/

void printTab(int num);
void printToken(Token *token);
void printTokenList(Token *tokens);
void printNode(Node *node, int tabs);
void printVar(Node *node);
void printFunction(Node *node);
void printProgram(Program *prog);

#endif