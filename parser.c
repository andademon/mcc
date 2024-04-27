#include "include/mcc.h"

/**
 * grammar in EBNF
 * 1.program -> declaration-list
 * 2.declaration-list −> declaration { declaration }
 * 3.declaration -> variable-declaration | function-declaration
 * 4.variable-declaration -> type-specifier init-declarator-list ;
 * 5.type-specifier −> char | int | void | struct-specifier
 * 6.init-declarator-list -> init-declarator { , init-declaractor }
 * 7.init-declarator -> declarator [ = initializer ]
 * new: 
 * 8.declarator -> [ pointer ] direct-declarator
 * 9.pointer -> * { * }
 * 10.direct-declarator -> ID
 * | ( declarator )
 * | direct-declarator { type-suffix }
 * 11.type-suffix -> "[" [ constant ] "]"
 * | "(" [ parameter-list ] ")"
 * | ε
 * 
 * 暂不支持direct-declarator -> direct-declarator ( identifier-list ) 缺省类型声明
 * 
 * 12.parameter-list -> parameter-declaration { , parameter-declaration }
 * 13.parameter-declaration -> type-specifiers declarator
 * | parameter-declaration -> type-specifiers abstract-declarator
 * 暂不支持abstract-declarator 抽象类型声明
 * 
 * n.abstract-declarator -> pointer
 * | [ pointer ] direct-abstract-declarator
 * n.direct-abstract-declarator ->
 * | ( abstract-declarator )
 * | ( abstract-declarator ) { direct-abstract-declarator } "[" [ constant ] "]"
 * | ( abstract-declarator ) { direct-abstract-declarator } ( parameter-type-list )
 * 
 * 
 * 
 * 14.initializer -> assignment-expression
 * | { initializer-list }
 * 取代下面这条
 * | "{" [ constant { , constant } ] "}"
 * 
 * 15.initiallizer-list -> initializer { , initializer }
 * 
 * 暂时不实现结构体
 * 6.struct-specifier −> struct [ID] [ { struct-declaration-list} ]
 * 7.struct-declaration-list ->
 * 
 * 16.function-declaration -> type-specifier declarator compound-statement
 * 暂不支持缺省函数返回类型
 * 
 * 以下三条舍弃，用12-13替换
 * 9.parameters −> parameter-list | void | ε
 * 10.parameter-list -> parameter { , parameter }
 * 11.parameter −> type-specifier declarator
 * 
 * 
 * 17.compound-statement −> { [local-declarations] [statement-list] }
 * 18.local-declarations −> { variable-declaration }
 * 19.statement-list −> { statement }
 * 20.statement −> compound-statement
 * | expression-statement
 * | selection-statement 
 * | labeled-statement
 * | iteration-statement 
 * | jump-statement
 * 21.expression-statement −> [ expression ] ;
 * 22.selection-statement −> if ( expression ) statement { else if ( expression ) statement } [ else statement ]
 * | switch ( expression ) statement
 * 23.iteration-statement −> while ( expression ) statement
 * | for ( [expression ] ; [expression ] ; [expression] ) statement
 * | do statement while ( expression ) ;
 * 24.jump-statement −> return [ expression] ;
 * | break ; 
 * | goto ID ;
 * 25.labeled-statement −> ID : statement
 * | case constant : statement
 * | default : statement
 * 26.expression −> assignment-expression {, assignment-expression}
 * 27.assignment-expression −> conditional-expression {assignment-operator conditional-expression}
 * 28.assignment-operator -> =
 * 29.conditional-expression −> logical-or-expression
 * | logical-or-expression ? expression : conditional-expression
 * 30.logical-or-expression -> logical-and-expression {|| logical-and-expression}
 * 31.logical-and-expression -> equality-expression {&& equality-expression}
 * 32.equality-expression -> relational-expression {equality-operator relational-expression}
 * 33.equality-operator -> == | !=
 * 34.relational-expression -> additive-expression {relational-operator additive-expression}
 * 35.relational-operator −> > | < | >= | <=
 * 36.additive-expression −> multiplicative-expression {add-operator multiplicative-expression }
 * 37.add-operator −> + | -
 * 38.multiplicative-expression −> unary-expression {mul-operator unary-expression}
 * 39.mul-operator −> * | /
 * 40.unary-expression -> postfix-expression
 * | ++ unary-expression
 * | -- unary-expression
 * | unary-operator unary-expression
 * 暂不支持sizeof
 * 41.unary-operator -> & | * | + | - | ~ | !
 * 42.postfix-expression -> primary-expression
 * | primary-expression ++
 * | primary-expression --
 * | primary-expression . identifier
 * | primary-expression -> identifier
 * | primary-expression "[" expression "]"
 * | primary-expression ( [ argument-expression-list ] )
 * | primary-expression { "[" expression "]" | . identifier | -> identifier | ++ | -- | ( argument-expression-list ) }
 * 暂不支持函数指针
 * 43.primary-expression −> identifier | constant | string | ( expression )
 * 45.constant -> NUM | CHAR
 * 46.argument-list −> assignment-expression { , assignment-expression }
*/

static Program *prog;
static Token *tokens;
static Token *current_token;

static void next_token();
static void expect(char *str);
static void expect_type(int type);
static bool match(char *str);
static bool match_type(int type);
static int get_op_type(char *op);
static int get_unary_op_type(char *op);

static Node *program();
static Vector *declaration_list();
static Node *declaration();
static Node *variable_declaration();
static Vector *init_declarator_list(Type *base_type);
static Node *init_declarator(Type *base_type);

// 返回值类型

/**
 * ID dir-dcl
 * *ID dcl
 * (*ID) dir-dcl
 * (*ID)[] dir-dcl
 * 声明时优先级：
 * 括号 > 后缀 > 前缀
 * 括号和后缀处于同一级别时，后缀 < 括号
 * dir-dcl []
 * 
 * 构造Type类型
 * 前缀 * -> pointer to
 * 后缀[] -> array of
 * 后缀() -> function return
 * 括号() 不改变内部元素类型，只是为了提升优先级
 * 
 * pfa: array of T0
 * T0: pointer to T1
 * T1: function return T2
 * T2: base_type
 * 
*/

static Type *type_specifier();
static Type *declarator(Type *base_type);

static Type *pointer(Type *base_type);
static Type *direct_declarator(Type *base_type);
static Type *type_suffix(Type *base_type);

static Vector *parameter_list();
static Node *parameter_declaration();

static Node *initializer();
static Vector *initiallizer_list();

static Node *struct_specifier();
static Node *struct_declaration_list();
static Node *function_declaration();
// static Vector *parameters();
static Vector *parameter_list();
// static Node *parameter();
static Node *compound_statement();
static Vector *local_declarations();
static Vector *statement_list();
static Node *statement();
static Node *expression_statement();
static Node *selection_statement();
static Node *iteration_statement();
static Node *jump_statement();
static Node *labeled_statement();
static Node *expression();
static Node *assignment_expression();
static Node *conditional_expression();
static Node *logical_or_expression();
static Node *logical_and_expression();
static Node *equality_expression();
static Node *relational_expression();
static Node *additive_expression();
static Node *multiplicative_expression();
static Node *unary_expression();
static Node *postfix_expression();
static Node *primary_expression();
static Vector *argument_list();
static Node *constant();

static Map *op_map;
static Map *unary_op_map;

void next_token () {
    current_token = current_token->next;
}

static void init_op_map() {
    op_map = new_map();
    map_puti(op_map, "+", OP_ADD);
    map_puti(op_map, "-", OP_SUB);
    map_puti(op_map, "*", OP_MUL);
    map_puti(op_map, "/", OP_DIV);
    map_puti(op_map, "%", OP_MOD);
    map_puti(op_map, "<", OP_LT);
    map_puti(op_map, "<=", OP_LE);
    map_puti(op_map, ">", OP_GT);
    map_puti(op_map, ">=", OP_GE);
    map_puti(op_map, "==", OP_EQ);
    map_puti(op_map, "!=", OP_NE);
    map_puti(op_map, "&&", OP_LOGAND);
    map_puti(op_map, "||", OP_LOGOR);
    map_puti(op_map, "=", OP_ASSIGN);
}

static void init_unary_op_map() {
    unary_op_map = new_map();
    map_puti(unary_op_map, "!", OP_NOT);
    map_puti(unary_op_map, "&", OP_ADDR);
    map_puti(unary_op_map, "*", OP_DEREF);
    map_puti(unary_op_map, "+", OP_PLUS);
    map_puti(unary_op_map, "-", OP_MINUS);
    map_puti(unary_op_map, "++", OP_INC);
    map_puti(unary_op_map, "--", OP_DEC);
    map_puti(unary_op_map, ".", OP_MEMBER);
    map_puti(unary_op_map, "->", OP_MEMBER);
}

static int get_op_type(char *op) {
    if (!op_map) init_op_map();
    return map_geti(op_map, op, -1);
}

static int get_unary_op_type(char *op) {
    if (!unary_op_map) init_unary_op_map();
    return map_geti(unary_op_map, op, -1);
}

Program *parse(Token *tokens)
{
    current_token = tokens;
    Program *prog = new_prog();
    Node *node = program();
    if (node && node->decls) {
        for(int i = 0;i < node->decls->len;i++) {
            Node *temp = node->decls->data[i];
            if(temp->node_type == ND_VAR_DECL) {
                vec_push(prog->gvars, temp);
            } else if (temp->node_type == ND_FUNC_DECL) {
                vec_push(prog->funcs, temp);
            }
        }
    }
    return prog;
}

static void expect(char *str) {
    Token *t = current_token;
    if (strcmp(t->value, str) == 0) {
        current_token = current_token->next;
        return;
    }
    printf("expect %s, current token: %d\t%s\n", str, current_token->id, current_token->value);
    exit(0);
}

static bool match(char *str) {
    if (strcmp(current_token->value, str) == 0) {
        return true;
    }
    return false;
}

static bool match_type(int type) {
    if (current_token->type == type) {
        return true;
    }
    return false;
}

static void expect_type(int type) {
    if (current_token->type == type) {
        current_token = current_token->next;
        return;
    }
    printf("expect %d, current token: %d\t%s\n", type, current_token->id, current_token->value);
    exit(0);
}

/* 1.program -> declaration-list */
static Node *program() {
    Node *program = new_node("Program", ND_PROGRAM);
    program->decls = declaration_list();
    return program;
}

/* 2.declaration-list −> declaration { declaration } */
static Vector *declaration_list() {
    Vector *decls = new_vec();
    while (!match_type(TK_EOF)) {
        Node *node = declaration();
        if (node == NULL) {
            break;
        }
        vec_push(decls, node);
    }
    return decls;
}

/* 3.declaration -> variable-declaration | function-declaration */
static Node *declaration() {
    Node *node = NULL;
    Token *token_bak = &(*current_token);
    node = function_declaration();
    /* backtrace */
    if (node == NULL) {
        current_token = token_bak;
        node = variable_declaration();
    }
    return node;
}

/**
 * 4.variable-declaration -> type-specifier init-declarator-list ;
*/
static Node *variable_declaration() {
    Type *type_spec = type_specifier();
    if (!type_spec) return NULL;

    Node *node = new_node("VariableDeclaration", ND_VAR_DECL);
    node->declarators = init_declarator_list(type_spec);

    expect(";");
    return node;
}

/**
 * 5.init-declarator-list -> init-declarator { , init-declaractor }
*/
static Vector *init_declarator_list(Type *base_type) {
    Vector *init_decl_list = new_vec();

    vec_push(init_decl_list, init_declarator(base_type));

    while(match(",")) {
        next_token();
        vec_push(init_decl_list, init_declarator(base_type));
    }

    return init_decl_list;
}

/* 根据declarator的类型区分node是function还是variable */

/**
 * 6.init-declarator -> declarator [ = initializer ]
*/
static Node *init_declarator(Type *base_type) {
    Node *node = new_node("VariableDeclarator", ND_VAR_DECLARATOR);
    Type *type = declarator(base_type);
    complete_type_size(type);
    node->type = type;
    // exit(0);
    node->name = type->name;
    
    if (match("=")) {
        next_token();
        node->init = initializer();
    }
    return node;
}

/**
 * 7.declarator -> [ pointer ] direct-declarator
*/
static Type *declarator(Type *base_type) {
    Type *ptr = NULL;
    if (match("*")) {
        ptr = pointer(base_type);
    }
    // 指针声明优先级低于direct-declarator,所以direct_declarator的基类是pointer_to
    if (ptr) return direct_declarator(ptr);
    return direct_declarator(base_type);
}

/**
 * int *(*(*(*e())[])())();
 * int                          *(*(*(*e())[])())()
 * ptr int                      (*(*(*e())[])())()
 * func ptr int                 (*(*(*e())[])())
 *                              *(*(*e())[])()
 * ptr func ptr int             (*(*e())[])()
 * func ptr func ptr int        (*(*e())[])
 *                              *(*e())[]
 * ptr func ptr func ptr int    (*e())[]
 * arr ptr func ptr func ptr int(*e())
 *                              *e()
 * ptr arr ptr func ptr func ptr int e()
 * func ptr arr ptr func ptr func ptr int e
 * 
 * int (*e())[]
 * 
 * int          (*e())[]
 * arr int      (*e())
 *              *e()
 * ptr arr int  e()
 * func ptr arr int e
 * 
 * func ptr arr int
*/

/**
 * 8.pointer -> * { * }
*/
static Type *pointer(Type *base_type) {
    expect("*");
    Type *type = new_type(TY_POINTER_TO, 8, 8);
    while(match("*")) {
        next_token();
        Type *base = type;
        type = new_type(TY_POINTER_TO, 8, 8);
        type->base = base;
    }
    if (base_type) {
        Type *temp = get_base(type);
        temp->base = base_type;
    }
    return type;
}

/**
 * int (*pfa)();
*/

/**
 * 9.direct-declarator -> ID type-suffix
 * | "(" declarator ")" type-suffix
*/
static Type *direct_declarator(Type *base_type) {
    // 基本类型name在这里如何表示
    if (match_type(IDENTIFIER)) {
        char *name = current_token->value;
        next_token();
        Type *dir_dcl;
        Type *suffix = type_suffix(base_type);

        // 如果ID有后缀运算符，则dir_dcl的类型是后缀运算符类型，且此时first type为dir_dcl本身
        // 如果ID无后缀运算符，则die_dcl类型是其基类，此时first type就是该基类
        if (suffix) {
            // Type *suffix_base = get_base(suffix);
            // suffix_base->base = base_type;
            dir_dcl = suffix;
        }
        else dir_dcl = base_type;
        dir_dcl->name = name;
        return dir_dcl;
    }
    else {
        expect("(");
        // 这里应该在后缀运算符之后，使用后缀运算符的类型作为基类
        Type *dcl = declarator(NULL);
        expect(")");

        // 在第二种情况下，dcl的基类是suffix -> base_type,如果无后缀运算符，则dcl基类为base_type
        Type *suffix = type_suffix(base_type);
        Type *dcl_base = get_base(dcl);
        if (suffix) {
            // Type *suffix_base = get_base(suffix);
            // suffix_base->base = base_type;
            dcl_base->base = suffix;
        }
        else dcl_base->base = base_type;
        return dcl;
    }
}

static Type *type_suffix(Type *base_type) {
    if (match("(")) {
        next_token();
        Type *type = new_type(TY_FUNC, 0, 0);
        type->params = parameter_list();
        expect(")");
        Type *suffix_base = get_base(type);
        suffix_base->base = base_type;
        return type;
    }
    else if(match("[")) {
        next_token();
        Type *type = new_type(TY_ARRAY_OF, 0, 0);
        if (match_type(NUMBER)) {
            type->array_len = str_to_int(current_token->value);
            next_token();
        }
        expect("]");

        Type *temp = type;
        while (match("[")) {
            next_token();
            temp->base = new_type(TY_ARRAY_OF, 0, 0);
            if (match_type(NUMBER)) {
                temp->base->array_len = str_to_int(current_token->value);
                next_token();
            }
            temp = temp->base;
            expect("]");
        }
        Type *suffix_base = get_base(type);
        suffix_base->base = base_type;
        return type;
    }
    return NULL;
}

/**
 * parameter-list -> parameter { , parameter }
*/
static Vector *parameter_list() {
    Vector *params = new_vec();
    Node *node = parameter_declaration();
    while (node) {
        vec_push(params, node);
        if (match(",")) {
            next_token();
            node = parameter_declaration();
        }
        else break;
    }
    return params;
}

/**
 * parameter-declaration -> type-specifiers declarator
*/
static Node *parameter_declaration() {
    Type *base_type = type_specifier();
    if (base_type == NULL) return NULL;

    Node *node = new_node("FunctionParam", ND_FUNC_PARAM);
    Type *type = declarator(base_type);
    // 注意在函数形参中，将第一类型是数组的元素作为指针处理
    if (type->kind == TY_ARRAY_OF) {
        type->kind = TY_POINTER_TO;
        type->align = 8;
        type->size = 8;
    }
    complete_type_size(type);
    node->type = type;
    node->name = type->name;
    return node;
}

/**
 * 14.initializer -> assignment-expression
 * | { initializer-list }
*/
static Node *initializer() {
    if (match("{")) {
        next_token();
        Node *node = new_node("ArrayInit", ND_ARR_EXPR);
        node->args = initiallizer_list();
        expect("}");
        return node;
    }
    else return assignment_expression();

    // if (match("{")) {
    //     next_token();
    //     Node *node = new_node("ArrayInit", ND_ARR_EXPR);
    //     int count = 0;
    //     while (match_type(NUMBER) || match_type(CHARACTER) || match_type(STRING)) {
    //         count++;
    //         vec_push(node->args, constant());
    //         if (match(",")) {
    //             next_token();
    //             continue;
    //         }
    //     }
    //     expect("}");
    //     node->len = count;
    //     return node;
    // }
    // else return assignment_expression();
}

/**
 * 15.initiallizer-list -> initializer { , initializer }
*/
static Vector *initiallizer_list() {
    Vector *vec = new_vec();
    Node *init = initializer();
    
    while (init) {
        vec_push(vec, init);
        if (match(",")) {
            next_token();
            init = initializer();
        }
        else break;
    }
    return vec;
}

/* 5.type-specifier −> int | void | struct-specifier */
static Type *type_specifier() {
    Token *token = &(*current_token);
    /* int | char | void  */
    if (match("int")) {
        next_token();
        Type *type = new_type(TY_INT, 4, 4);
        return type;
    }
    else if (match("char")) {
        next_token();
        Type *type = new_type(TY_CHAR, 1, 1);
        return type;
    }
    else if (match("void")) {
        next_token();
        Type *type = new_type(TY_VOID, 0, 0);
        return type;
    }
    else if (match("struct")){
        // Node *node = struct_specifier();
        struct_specifier();
        /**
         * TODO: struct type
        */
        // return new_node("type_spec", ND_TYPE_SPEC);
    }
    return NULL;
}

/* 6.struct-specifier −> struct [ID] [ { struct-declaration-list } ] */
static Node *struct_specifier() {
    expect("struct");
    expect_type(IDENTIFIER);
    if (match("{")) {
        next_token();
        struct_declaration_list();
        expect("}");
    }
}
/* 7.struct-declaration-list -> */
/**
 * TODO: 补充
*/
static Node *struct_declaration_list() {

}

/* 8.function-declaration -> type-specifier ID ( parameters ) compound-statement */
/* 16.function-declaration -> type-specifier declarator compound-statement */
static Node *function_declaration() {
    // Node *node = NULL;
    // Token *id = NULL;
    // Vector *params = NULL;
    // Node *body = NULL;

    Type *base_type = type_specifier();
    if (base_type == NULL) {
        return NULL;
    }
    Node *func = new_node("FunctionDeclaration", ND_FUNC_DECL);
    Type *type = declarator(base_type);
    if (type->kind != TY_FUNC) {
        return NULL;
    }
    complete_type_size(type);
    func->type = type;
    func->body = compound_statement();
    func->params = type->params;
    func->name = type->name;

    return func;

    // if (match_type(IDENTIFIER)) {
    //     id = &(*current_token);
    //     next_token();
    //     if (match("(")) {
    //         next_token();
    //         params = parameters();
    //         if (match(")")) {
    //             next_token();
    //         }
    //         else return NULL;
    //     }
    //     else return NULL;
    // }
    // body = compound_statement();
    
    // func->id = id;
    // if (params != NULL)
    //     func->params = params;
    // func->body = body;
    // return func;
}

/* 9.parameters −> parameter-list | void | ε */
// static Vector *parameters() {
//     if (match(")")) {
//         return NULL;
//     }
//     if (match("void")) {
//         next_token();
//         return NULL;
//     }
//     return parameter_list();
// }

/* 10.parameter-list -> parameter { , parameter } */
// static Vector *parameter_list() {
//     Vector *params = new_vec();
//     vec_push(params, parameter());    
        
//     while (match(",")) {
//         next_token();
//         vec_push(params, parameter());
//     }

//     return params;
// }

/* 11.parameter −> type-specifier [ "*" ] ID [ “[” “]” ] */
// static Node *parameter() {
//     Node *param = new_node("FunctionParam", ND_FUNC_PARAM);
//     Node *type_spec = type_specifier();
//     if (match("*")) {
//         next_token();
//         param->is_pointer = true;
//     }

//     Token *id = &(*current_token);
//     expect_type(IDENTIFIER);
    
//     param->id = id;
//     param->base_type = type_spec->base_type;
//     if (match("[")) {
//         next_token();
//         expect("]");
//         param->is_array = true;
//         param->len = 1;
//     }
//     return param;
// }

/* 12.compound-statement −> { [local-declarations] [statement-list] } */
static Node *compound_statement() {
    Node *block_stmt = new_node("BlockStmt", ND_BLOCK);
    expect("{");
    Vector *decls = local_declarations();
    Vector *stmts = statement_list();
    expect("}");
    block_stmt->decls = decls;
    block_stmt->stmts = stmts;
    return block_stmt;
}

/* 13.local-declarations −> { variable-declaration } */
/**
 * TODO: 将decls串联起来并返回(已完成)
*/
static Vector *local_declarations() {
    Vector *decls = new_vec();
    for(;;) {
        Node *var_decl = variable_declaration();
        if (var_decl == NULL) {
            break;
        }
        vec_push(decls, var_decl);
    }
    return decls;
}

/* 14.statement-list −> { statement } */
static Vector *statement_list() {
    Vector *stmts = new_vec();
    if (match("}")) {
        return stmts;
    }
    Node *p = NULL;
    for(;;) {
        Node *stmt = statement();
        if (stmt == NULL) {
            break;
        }
        vec_push(stmts, stmt);
        if (match("}")) {
            break;
        }
    }
    return stmts;
}

/**
 * 15.statement −> compound-statement 
 * | expression-statement
 * | selection-statement 
 * | labeled-statement
 * | iteration-statement 
 * | jump-statement
 * 
 * TODO: 补充完整
*/
static Node *statement() {
    if (match("if") || match("switch")) {
        return selection_statement();
    }
    else if (match("while") || match("for") || match("do")) {
        return iteration_statement();
    }
    else if (match("return") || match("break") || match("continue") || match("goto")) {
        return jump_statement();
    }
    else if (match("case") || match("default")) {
        return labeled_statement();
    }
    else if (match("{")) {
        return compound_statement();
    }
    else {
        if (current_token->next && strcmp(current_token->next->value, ":") == 0) {
            return labeled_statement();
        }
        else {
            return expression_statement();
        }
    }
}

/* 16.expression-statement −> [ expression ] ; */
static Node *expression_statement() {
    Node *expr = expression();
    if (expr == NULL) {
        return NULL;
    }
    expect(";");
    Node *exprStmt = new_node("ExprStmt", ND_EXPR_STMT);
    exprStmt->body = expr;
    return exprStmt;
}

/**
 * 17.selection-statement −> if ( expression ) statement {else if (expression) statement} [ else statement ]
 * | switch ( expression ) statement
 * 
 * TODO: 将switch-statement特殊化
 * switch ( expression ) "{" { label-statement } "}"
*/
static Node *selection_statement() {
    if (match("if")) {
        Node *if_stmt = new_node("IfStmt", ND_IF_STMT);
        Node *els = NULL;
        next_token();
        expect("(");
        Node *test = expression();
        if (test == NULL) {
            printf("if statement must have condition;\n");
            exit(0);
        }
        expect(")");
        Node *then = statement();
        if (match("else")) {
            next_token();
            els = statement();
        }
        if_stmt->test = test;
        if_stmt->then = then;
        if_stmt->els = els;

        return if_stmt;
    }
    else if (match("switch")) {
        next_token();
        Node *switch_stmt = new_node("SwitchStmt", ND_SWITCH_STMT);
        expect("(");
        switch_stmt->test = expression();
        expect(")");
        // switch_stmt->body = statement();
        expect("{");
        
        while (!match("}")) {
            Node *case_stmt = labeled_statement();
            if (case_stmt != NULL) {
                vec_push(switch_stmt->cases, case_stmt);
            }
        }
        expect("}");
        return switch_stmt;
    }
}

/**
 * 18.iteration-statement −> while ( expression ) statement
 * | for ( [expression ] ; [expression ] ; [expression] ) statement
 * | do statement while ( expression ) ;
*/
static Node *iteration_statement() {
    if (match("while")) {
        next_token();
        expect("(");
        Node *test = expression();
        if (test == NULL) {
            printf("while statement must have condition expression");
        }
        expect(")");
        Node *body = statement();
        Node *while_stmt = new_node("WhileStmt", ND_WHILE_STMT);
        while_stmt->test = test;
        while_stmt->body = body;
        return while_stmt;
    }
    else if (match("do")) {
        next_token();
        Node *expr = new_node("DoWhileStmt", ND_DO_WHILE);
        expr->body = statement();
        expect("while");
        expect("(");
        expr->test = expression();
        expect(")");
        expect(";");
        return expr;
    }
    else if (match("for")) {
        next_token();
        expect("(");
        // TODO: for循环的init部分暂时不能支持局部变量声明
        // Node *init = variable_declaration();
        Node *init = expression();
        // if (init == NULL) expect(";"); //如果init不是NULL说明;已被读取
        expect(";");
        Node *test = expression();
        expect(";");
        Node *update = expression();
        expect(")");
        Node *body = statement();
        Node *for_stmt = new_node("ForStmt", ND_FOR_STMT);
        for_stmt->init = init;
        for_stmt->test = test;
        for_stmt->update = update;
        for_stmt->body = body;
        return for_stmt;
    }
}

/**
 * 19.jump-statement −> return [ expression ] ;
 * | break ; 
 * | goto ID ;
*/
static Node *jump_statement() {
    if (match("return")) {
        next_token();
        Node *return_stmt = new_node("ReturnStmt", ND_RETURN_STMT);
        Node *body = expression();
        expect(";");
        return_stmt->body = body;
        return return_stmt;
    }
    else if (match("break")) {
        next_token();
        expect(";");
        return new_node("BreakStmt", ND_BREAK_STMT);
    }
    else if (match("continue")) {
        next_token();
        expect(";");
        return new_node("ContinueStmt", ND_CONTINUE_STMT);
    } 
    else {
        expect("goto");
        expect_type(IDENTIFIER);
        expect(";");
        return new_node("GotoStmt", ND_GOTO_STMT);
    }
}

/**
 * 20.labeled-statement −> ID : statement
 * | case conditional-expression : statement
 * | default : statement
 * 
 * TODO: 这里是不是要改一下，防止平级case识别为嵌套
 * 
 * labeled-statement -> ID : statement
 * | case primary-expression : statement-list
 * | default : statement-list
 * 
 * 已确认，不需要修改文法，严格按照原文法更容易处理
*/
static Node *labeled_statement() {
    if (match("case")) {
        next_token();
        Node *node = new_node("Case", ND_CASE);
        node->test = constant();
        expect(":");
        node->then = statement();
        return node;
    }
    else if (match("default")) {
        next_token();
        Node *node = new_node("Case", ND_CASE);
        node->test = NULL;
        expect(":");
        node->then = statement();
        return node;
    }
    else {
        expect_type(IDENTIFIER);
        expect(":");
        statement();
    }
}

/* 21.expression −> assignment-expression
 * | unary-expression
 * | binary-expression
 * | conditional-expression
 * | update-expression
 * | logical-expression
 * | primary-expression
 * 
 * expression -> assignment-expression
 * | conditional-expression
 * 
 * unary-expression -> + | - | ! expr
 * binary-expression -> expr binary-operator expr
 * conditional-expression -> expr ? expr : expr
 * update-expression -> ++ | -- variable
 * | variable ++ | --
 * logical-expression -> expr logical-operator expr
 * 
 */
// static Node *expression() {
//     Token *token_bak = &(*current_token);
//     Node *expr = assignment_expression();
//     if (expr == NULL) {
//         current_token = token_bak;
//         expr = conditional_expression();
//     }
//     return expr;
// }

static Node *expression() {
    Node *expr = assignment_expression();
    if (match(",")) {
        Node *sequenceExpr = new_node("SequenceExpression", ND_SEQUENCE_EXPR);
        vec_push(sequenceExpr->exprs, expr);
        while (match(",")) {
            next_token();
            Node *new_expr = assignment_expression();
            vec_push(sequenceExpr->exprs, new_expr);
        }
        return sequenceExpr;
    }
    return expr;
}

/**
 * 22.assignment-expression −> variable = expression
*/
static Node *assignment_expression() {
    Node *left = conditional_expression();

    while (match("=")) {
        Token *token = &(*current_token);
        next_token();

        Node *right = conditional_expression();
        Node *left_bak = left;
        left = new_node("binaryExpr", ND_BINARY_EXPR);
        left->lhs = left_bak;
        left->rhs = right;
        left->token = token;
        left->op_type = get_op_type(token->value);
    }
    return left;
}

/**
 * 24.conditional-expression −> additive-expression [relation-operator additive-expression ]
 * 25.relation-operator −> <= | < | > | >= | != | ==
*/
// static Node *conditional_expression() {
//     Node *left = additive_expression();
//     Node *right = NULL;
//     if (left == NULL) {
//         return NULL;
//     }
//     if (match("!=")
//         || match("==")
//         || match(">")
//         || match(">=")
//         || match("<")
//         || match("<=")
//     ) {
//         Token *token = &(*current_token);
//         next_token();
//         right = additive_expression();
//         if (right == NULL) {
//             return NULL;
//         }
//         else {
//             Node *node = new_node("BinaryExpr", ND_BINARY_EXPR);
//             node->lhs = left;
//             node->rhs = right;
//             node->token = op;
//             node->op_type = get_op_type(op->value);
//             return node;
//         }
//     }
//     return left;
// }

static Node *conditional_expression() {
    Node *expr = logical_or_expression();
    if (match("?")) {
        next_token();
        Node *then = expression();
        expect(":");
        Node *els = conditional_expression();
        Node *expr_bak = expr;
        expr = new_node("TernaryExpression", ND_TERNARY_EXPR);
        expr->test = expr_bak;
        expr->then = then;
        expr->els = els;
    }
    return expr;
}

static Node *logical_or_expression() {
    Node *left = logical_and_expression();

    while (match("||")) {
        Token *token = &(*current_token);
        next_token();
        Node *right = logical_and_expression();
        if (right != NULL) {
            Node *left_bak = left;
            left = new_node("binaryExpr", ND_BINARY_EXPR);
            left->lhs = left_bak;
            left->rhs = right;
            left->token = token;
            left->op_type = get_op_type(token->value);
        }
    }
    return left;
}

static Node *logical_and_expression() {
    Node *left = equality_expression();

    while (match("&&")) {
        Token *token = &(*current_token);
        next_token();
        Node *right = equality_expression();
        if (right != NULL) {
            Node *left_bak = left;
            left = new_node("binaryExpr", ND_BINARY_EXPR);
            left->lhs = left_bak;
            left->rhs = right;
            left->token = token;
            left->op_type = get_op_type(token->value);
        }
    }
    return left;
}

static Node *equality_expression() {
    Node *left = relational_expression();

    while (match("==") || match("!=")) {
        Token *token = &(*current_token);
        next_token();
        Node *right = relational_expression();
        if (right != NULL) {
            Node *left_bak = left;
            left = new_node("binaryExpr", ND_BINARY_EXPR);
            left->lhs = left_bak;
            left->rhs = right;
            left->token = token;
            left->op_type = get_op_type(token->value);
        }
    }
    return left;
}

static Node *relational_expression() {
    Node *left = additive_expression();

    while (match(">") 
        || match("<") 
        || match(">=") 
        || match("<=")
    ) {
        Token *token = &(*current_token);
        next_token();
        Node *right = additive_expression();
        if (right != NULL) {
            Node *left_bak = left;
            left = new_node("binaryExpr", ND_BINARY_EXPR);
            left->lhs = left_bak;
            left->rhs = right;
            left->token = token;
            left->op_type = get_op_type(token->value);
        }
    }
    return left;
}

/**
 * 26.additive-expression −> multiplicative-expression {add-operator multiplicative-expression }
 * 27.add-operator −> + | -
 * TODO: 连加
*/
static Node *additive_expression() {
    Node *left = multiplicative_expression();
    if (left == NULL) {
        return NULL;
    }
    while (match("+") || match("-")) {
        Token *token = &(*current_token);
        next_token();
        Node *right = multiplicative_expression();
        if (right != NULL) {
            Node *left_bak = left;
            left = new_node("binaryExpr", ND_BINARY_EXPR);
            left->lhs = left_bak;
            left->rhs = right;
            left->token = token;
            left->op_type = get_op_type(token->value);
        }
    }
    return left;
}

/**
 * 28.multiplicative-expression −> primary-expression {mul-operator primary-expression}
 * 29.mul-operator −> * | / | %
 * TODO: fix
 * 如何处理连加 / 连乘
*/
static Node *multiplicative_expression() {
    Node *left = unary_expression();
    if (left == NULL) {
        return NULL;
    }
    while (match("*") || match("/") || match("%")) {
        Token *token = &(*current_token);
        next_token();
        Node *right = unary_expression();
        if (right != NULL) {
            Node *left_bak = left;
            left = new_node("binaryExpr", ND_BINARY_EXPR);
            left->lhs = left_bak;
            left->rhs = right;
            left->token = token;
            left->op_type = get_op_type(token->value);
        }
    }
    return left;
}

static Node *unary_expression() {
    if (match("++")
    || match("--")
    || match("&")
    || match("*")
    || match("+")
    || match("-")
    || match("~")
    || match("!")
    ) {
        Token *token = &(*current_token);
        next_token();
        Node *node = new_node("UnaryExpression", ND_UNARY_EXPR);
        node->token = token;
        node->is_prefix = true;
        node->body = unary_expression();
        node->op_type = get_unary_op_type(token->value);
        return node;
    }
    else return postfix_expression();
}
static Node *postfix_expression() {
    Node *node = primary_expression();

    while (match("++")
        || match("--")
        || match("[")
        || match(".")
        || match("->")
        || match("(")
    ) {
        Token *token = &(*current_token);
        Node *bak = node;

        // 最先解析到的UnaryExpr先于后解析到的UnaryExpr执行，所以在语法树结构上先解析到的UnaryExpr处于底端
        if (match("++") || match("--")) {
            next_token();
            node = new_node("UnaryExpression", ND_UNARY_EXPR);
            node->body = bak;
            node->token = token;
            node->is_prefix = false;
            node->op_type = get_unary_op_type(token->value);
        }
        else if (match(".") || match("->")) {
            next_token();
            node = new_node("UnaryExpression", ND_UNARY_EXPR);
            node->body = bak;
            node->token = token;
            node->is_prefix = false;
            node->op_type = get_unary_op_type(token->value);
        }
        else if (match("[")) {
            next_token();
            node = new_node("UnaryExpression", ND_UNARY_EXPR);
            node->body = bak;
            node->token = token;
            node->is_prefix = false;
            Node *expr = expression();
            node->expression = expr;
            node->op_type = OP_ARR_MEMBER;
            expect("]");
        }
        else if (match("(")) {
            next_token();
            Vector *args = argument_list();
            expect(")");
            node = new_node("CallExpr", ND_CALLEXPR);
            node->callee = bak;
            node->args = args;
        }
    }
    return node;
}

/**
 * 30.primary-expression −> ID | constant | STRING | ( expression ) 
*/
static Node *primary_expression() {
    if (match_type(IDENTIFIER)) {
        Token *token = &(*current_token);
        next_token();
        Node *node = new_node("Identifier", ND_IDENT);
        node->token = token;
        return node;
    }
    else if (match_type(STRING)) {
        Token *token = &(*current_token);
        next_token();
        Node *node = new_node("Constant", ND_STR);
        node->token = token;
        return node;
    }
    else if (match_type(NUMBER) || match_type(CHARACTER)) {
        return constant();
    }
    else if (match("(")){
        expect("(");
        Node *expr = expression();
        expect(")");
        return expr;
    }
}

/**
 * constant -> NUM | CHAR
*/
static Node *constant() {
    Token *token = &(*current_token);
    if (match_type(NUMBER)) {
        next_token();
        Node *node = new_node("Constant", ND_NUM);
        node->token = token;
        return node;
    }
    else if (match_type(CHARACTER)) {
        next_token();
        Node *node = new_node("Constant", ND_CHAR);
        node->token = token;
        return node;
    }
    printf("parse error!\n");
    exit(1);
}

/**
 * 31.call-function −> ID ( [ argument-list ] )
*/
// static Node *call_function() {
//     if (match_type(IDENTIFIER)) {
//         Token *token = &(*current_token);
//         next_token();
//         if (match("(")) {
//             next_token();
//             Vector *args = argument_list();
//             if (match(")")) {
//                 next_token();
//                 Node *funcall = new_node("CallExpr", ND_CALLEXPR);
//                 funcall->id = token;
//                 funcall->args = args;
//                 return funcall;
//             }
//         }
//     }
//     return NULL;
// }

/**
 * 32.argument-list −> expression { , expression }
*/
static Vector *argument_list() {
    Vector *args = new_vec();
    Node *expr = assignment_expression();
    if (expr == NULL) {
        return args;
    }
    vec_push(args, expr);
    while (match(",")) {
        next_token();
        Node *new_node = assignment_expression();
        if (new_node == NULL) {
            break;
        }
        vec_push(args, new_node);
    }
    return args;
}

typedef struct {
    int op_type;
    int precedence; // 优先级
    int associativity; // 结合性 LEFT | RIGHT
} Operator;

// static Operator init_operator(int op_type, int precedence, int associativity) {
// }

// static Node *parse_expr(Node *node) {
//     return parse_expr_1(node->lhs, 0);
// }

// static Node *parse_expr_1(Node *node, int min_prec) {

// }