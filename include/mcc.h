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

typedef struct File File;
typedef struct Vector Vector;
typedef struct Map Map;
typedef struct Token Token;
typedef struct Node Node;
typedef struct Type Type;
typedef struct Program Program;
typedef struct Function Function;
typedef struct Var Var;
typedef struct SymbolTable SymbolTable;
typedef struct Reg Reg;
typedef struct BB BB;
typedef struct IR IR;

struct File {
    char *filename;
    char *path;
    char *content;
};

File *new_file(char *filename, char *path);

/* 工具类 */
struct Vector {
  void **data;
  int capacity;
  int len;
};

Vector *new_vec(void);
void vec_push(Vector *v, void *elem);
void vec_pushi(Vector *v, int val);
void *vec_pop(Vector *v);
void *vec_last(Vector *v);
bool vec_contains(Vector *v, void *elem);

struct Map {
    Vector *keys;
    Vector *vals;
};

Map *new_map(void);
void map_put(Map *map, char *key, void *val);
void map_puti(Map *map, char *key, int val);
void *map_get(Map *map, char *key);
int map_geti(Map *map, char *key, int default_);



/* (Alpha | _)(Alpha | Digit | _)* */
/* (Digit)(Digit)* */
/* "\"(* except for \")\"" */
typedef enum {
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
struct Token {
    int id;
    int line;
    int token_type;
    char *value;
    Token *next;
};

static Token *new_token(int id, int line, int token_type, char *value);
Token *Lexer(char *str);

char *new_str();
char *str_push(char *str, char c);
char *str_pop(char *str);


/* ---parser.c--- */

// AST node type
enum {
    ND_NULL_EXPR, // Do nothing

    ND_BLOCK,     // { ... }
    ND_CASE,      // case
    ND_GOTO_STMT,      // "goto"
    ND_LABEL_STMT,
    ND_CONTINUE_STMT, // continue
    ND_BREAK_STMT, // break
    ND_RETURN_STMT,    // return
  
    ND_VAR_DECL, // Variable declaration
    ND_TYPE_SPEC, // type specifier
    ND_VAR_DECLARATOR,

    ND_FUNC_DECL, // Function decl
    ND_FUNC_PARAM,

    ND_IF_STMT,
    ND_SWITCH_STMT,
    ND_WHILE_STMT,
    ND_DO_WHILE,
    ND_FOR_STMT,
    ND_EXPR_STMT, // Expression statement

    ND_UNARY_EXPR, // unary expression
    ND_BINARY_EXPR, // binary expression
    ND_TERNARY_EXPR, // ternary expression
    ND_SEQUENCE_EXPR, // sequence expression

    ND_NUM,       // literal Number
    ND_CHAR,
    ND_STR,
    ND_IDENT,
    ND_CALLEXPR,   // Function call

    ND_ARR_EXPR,  // array init
    ND_PROGRAM
};

/* op_type */
enum {
    // OP_COND = 0,      // ?:

    OP_ADD = 0,   // +
    OP_SUB,       // -
    OP_MUL,       // *
    OP_DIV,       // /
    OP_MOD,       // %
    OP_EQ,        // ==
    OP_NE,        // !=
    OP_LT,        // <
    OP_LE,        // <=
    OP_GT,       // >
    OP_GE,       // >=
    OP_LOGAND,    // &&
    OP_LOGOR,     // ||
    OP_ASSIGN,    // =

    OP_PLUS,      // unary +
    OP_MINUS,     // unary -
    OP_INC,       // increment ++
    OP_DEC,       // decrement --
    OP_MEMBER,    // . (struct member access)
    OP_ARR_MEMBER,  // arr [ expr ] array member access
    OP_ADDR,      // unary &
    OP_DEREF,     // unary *
    OP_NOT,       // !
    
    // OP_BITAND,    // &
    // OP_BITOR,     // |
    // OP_BITXOR,    // ^
    // OP_SHL,       // <<
    // OP_SHR,       // >>
    
    

    // OP_COMMA,     // ,
    // OP_BITNOT,    // ~
    
};

typedef enum {
    TY_NULL = 0,
    TY_VOID,
    TY_CHAR,
    TY_INT,
    TY_POINTER_TO,
    TY_ARRAY_OF,
    TY_FUNC,
    TY_STRUCT,
} TypeKind;

struct Type {
    int kind;
    int size;           // sizeof() value // memory size
    int align;          // alignment
    Type *origin;       // for type compatibility check

    // Pointer-to or array-of type. We intentionally use the same member
    // to represent pointer/array duality in C.
    //
    // In many contexts in which a pointer is expected, we examine this
    // member instead of "kind" member to determine whether a type is a
    // pointer or not. That means in many contexts "array of T" is
    // naturally handled as if it were "pointer to T", as required by
    // the C spec.
    Type *base;

    // Declaration
    char *name;

    // Array
    int array_len;
    // Function type
    // Type *return_type;
    //   Type *params;
    //   Type *next;
    Vector *params;

    // struct
    Vector *members;
};

struct Node {
    char *type_name;
    int node_type;

    // operator
    int op_type;
    char *name;
    Token *token;
    Type *type;
    
    bool is_prefix; // for unary-expr && postfix-expr op

    // if ( test ) then else els
    // for ( init ; test ; update) body
    // while ( test ) body
    // switch ( test ) body
    // case var: then
    // test ? then :els
    struct Node *body;
    struct Node *test;
    struct Node *then;
    struct Node *els;
    struct Node *init;
    struct Node *update;
    struct Node *callee;
    // BinaryExpr
    struct Node *lhs;
    struct Node *rhs;
    // ExprStmt / array_name [ expression ] in UnaryExpression
    struct Node *expression;

    Vector *decls;
    Vector *stmts;
    Vector *cases; // switch-case
    Vector *params; // function decl
    Vector *args; // function call, array init
    Vector *declarators;

    Vector *exprs; // sequence expression

    SymbolTable *scope; // 用于ND_BLOCK语义分析和代码生成
};

Node *new_node(char *type_name, int node_type);

// typedef struct {
//     NODE_TYPE type;
//     Node *exprs;
// } SequenceExpr;

struct Program {
  Vector *gvars;
  Vector *funcs;
};

Program *new_prog();

struct Function {
  char *name;
  Node *node;
  Vector *params;
  Vector *lvars;
  Vector *stmts;
  Vector *bbs;
};

Function *new_func();

struct Var {
    // 基本属性
    char *name;
    // int type;
    // int type;
    // bool is_array;
    // bool is_pointer;
    bool is_gval;
    bool is_param;
    Type *type;
    // int len; // if is array array.length else 1
    Node *init;

    // 计算属性，且有计算顺序要求
    // int type_size; // base type size
    // int memory_size; // memory size( such as array.memory_size = type_size * len, pointer.memory_size = 8 )    
    int offset; // offset from fp pointer
};

Var *new_var();

Program *parse(Token *tokens);

// sema.c

/* 符号表 */
struct SymbolTable {
    int scopeLevel; // 作用域层级(全局作用域层级为0)
    int scopeId; // 当前作用域在parent作用域中的id(0开始,递增)
    Map *entries;
    struct SymbolTable *parent;
    Vector *children;
};

SymbolTable *createSymbolTable(int scopeLevel, SymbolTable *parent);
SymbolTable *enterScope(SymbolTable *parent);
SymbolTable *exitScope(SymbolTable *current);
void insert(SymbolTable *node, char *key, void *val);
void *lookup_current(SymbolTable *node, char *key);
void *lookup(SymbolTable *node, char *name);
void sema_error(char *msg);
SymbolTable *buildSymbolTable(Program *prog);
SymbolTable *sema(Program *prog);

// codegen.c

void codegen(Program *prog, SymbolTable *table);

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

struct Reg {
    int vn; // virtual register number
    int rn; // real register number

    bool using;
};

/* 基本块 */
struct BB {
    int label;
    Vector *ir;

    Vector *in_regs;
    Vector *out_regs;
}; /*basic block*/

/* 三地址代码形式的IR */
struct IR {
    int op;
    Reg *r0;
    Reg *r1;
    Reg *r2;

    int ir_type;

    BB *bb1;
    BB *bb2;

    // Load/store size in bytes
    int size;
};

void printTab(int num);
void printToken(Token *token);
void printTokenList(Token *tokens);
void printNode(Node *node, int tabs);
void printProgram(Program *prog);
void printSymbolTable(SymbolTable *table, int tabs);

// 将语法树结构转换为更易处理的var + function
Program *tree_to_prog(Program *prog);
int str_to_int(char *str);
int get_type_size(int type);
char *get_size_name(int size);
int compute_var_memory_size(Var *var);
char *get_base_type_name(int type);
char *get_access_unit(int size);
// char *get_load_unit(int size);
// char *get_store_unit(int size);
Type *new_type(int kind, int size, int align);
void printType(Type *type, int tabs);
void printFullType(Type *type, int tabs);
void walk_stmt(Node *node, SymbolTable *table);
void complete_type_size(Type *type);
int compute_max_size(SymbolTable *table);
bool is_lval(Node *node);
bool is_compatible(Type *t1, Type *t2);
Type *get_base(Type *type);
Type *type_expr(Node *node, SymbolTable *table);
void type_stmt(Node *node, SymbolTable *table);
void check_type(Program *prog, SymbolTable *table);

#endif