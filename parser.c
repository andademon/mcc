#include "include/mcc.h"
#include <stdlib.h>

/**
 * grammar in EBNF
 * 1.program -> declaration-list
 * 2.declaration-list −> declaration { declaration }
 * 3.declaration -> variable-declaration | function-declaration
 * 4.variable-declaration −> type-specifier ID [ “[” NUM “]” ] ;
 * 5.type-specifier −> int | void | struct-specifier
 * 6.struct-specifier −> struct [ID] [ { struct-declaration-list} ]
 * 7.struct-declaration-list ->
 * 8.function-declaration -> type-specifier ID ( parameters ) compound-statement
 * 9.parameters −> parameter-list | void
 * 10.parameter-list -> parameter { , parameter }
 * 11.parameter −> type-specifier ID [ “[” “]” ]
 * 12.compound-statement −> { [local-declarations] [statement-list] }
 * 13.local-declarations −> { variable-declaration }
 * 14.statement-list −> { statement }
 * 15.statement −> compound-statement
 * | expression-statement
 * | selection-statement 
 * | labeled-statement
 * | iteration-statement 
 * | jump-statement
 * 16.expression-statement −> [ expression ]
 * 17.selection-statement −> if ( expression ) statement { else if ( expression ) statement } [ else statement ]
 * | switch ( expression ) statement
 * 18.iteration-statement −> while ( expression ) statement
 * | for ( [expression ] ; [expression ] ; [expression] ) statement
 * 19.jump-statement −> return [ expression] ;
 * | break ; 
 * | goto ID ;
 * 20.labeled-statement −> ID : statement
 * | case conditional-expression : statement
 * | default : statement
 * 21.expression −> assignment-expression
 * | conditional-expression
 * 22.assignment-expression −> variable = expression
 * 23.variable −> ID [ “[” NUM “]” | . ID ]
 * 24.conditional-expression −> additive-expression [relation-operator additive-expression ]
 * 25.relation-operator −> <= | < | > | >= | != | ==
 * 26.additive-expression −> multiplicative-expression {add-operator multiplicative-expression }
 * 27.add-operator −> + | -
 * 28.multiplicative-expression −> primary-expression {mul-operator primary-expression}
 * 29.mul-operator −> * | /
 * 30.primary-expression −> variable | NUM | ( expression ) | call-function
 * 31.call-function −> ID ( [ argument-list ] )
 * 32.argument-list −> expression { , expression }
*/

static Program *prog;
static Token *tokens;
static Token *current_token;
// static Token *token_bak;
static int decl_error_sign = 0;

static Node *new_node(char *type_name, NODE_TYPE type) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->type_name = type_name;
    node->type = type;
    return node;
}

void next_token () {
    current_token = current_token->next;
}

Program *parse(Token *tokens)
{
    current_token = tokens;
    return program();
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
    Token *t = current_token;
    if (strcmp(t->value, str) == 0) {
        return true;
    }
    return false;
}

static bool match_type(int type) {
    Token *t = current_token;
    if (current_token->type == type) {
        return true;
    }
    return false;
}

static void expectType(int type) {
    if (current_token->type == type) {
        printf("parse success: %s\n", current_token->value);
        current_token = current_token->next;
        return;
    }
    printf("expect %d, current token: %d\t%s\n", type, current_token->id, current_token->value);
    exit(0);
}

static bool equal(char *str) {
    if (strcmp(current_token->value, str) != 0) {
        return false;
    }
    current_token = current_token->next;
    return true;
}

static bool equalType(int type) {
    if (current_token->type != type) {
        return false;
    }
    current_token = current_token->next;
    return true;
}

/* 1.program -> declaration-list */
static Program *program() {
    Node *body = declaration_list();
    Program *program = (Program*)malloc(sizeof(Program));
    program->type = ND_PROGRAM;
    program->body = body;
    return program;
}

/* 2.declaration-list −> declaration { declaration } */
/* return node type: var_decl | function_decl list as program's body*/
static Node *declaration_list() {
    Node *decl_list = new_node("DeclarationList", ND_DECL_LIST);
    Node *head = new_node("Declaration", ND_NULL_EXPR);
    Node *p = head;
    while (!match_type(TK_EOF)) {
        Node *new_node = declaration();
        if (new_node == NULL) {
            break;
        }
        p->next = new_node;
        p = p->next;
    }
    decl_list->body = head->next;
    decl_list->is_list = true;
    return decl_list;
}

/* 3.declaration -> variable-declaration | function-declaration */
/* return node type: single var_decl | function_decl node */
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

/* 4.variable-declaration −> type-specifier ID [ “[” NUM “]” ] ; */
/* return node type: var_decl */
static Node *variable_declaration() {
    Node *node = NULL;
    /* type_specifier */
    Token *tok_bak = &(*current_token);
    Node *type_specifier_node = type_specifier();
    if (type_specifier_node == NULL) {
        current_token = tok_bak;
        return NULL;
    }
    /* ID */
    if (match_type(IDENTIFIER)) {
        next_token();
        if (match("[")) {
            next_token();
            if (match_type(NUMBER)) {
                next_token();
                if (match("]")) {
                    next_token();
                }
            }
        }
        /* ; */
        if (match(";")) {
            next_token();
            // node = (Node*)malloc(sizeof(Node));
            node = new_node("VarDecl", ND_VAR_DECL);
            return node;
        }
    }
    current_token = tok_bak;
    return NULL;
    // if (equal("[")) {
    //     expectType(NUMBER);
    //     expect("]");
    // }
    // expect(";");
}

/* 5.type-specifier −> int | void | struct-specifier */
/* return node type: single type_decl node */
static Node *type_specifier() {
    /* expect */
    /* void | int | float | double | short | long  */
    if (equal("int")) {
        return new_node("type_decl", ND_TYPE_DECL);
    }
    else if (equal("void")) {
        return new_node("type_decl", ND_TYPE_DECL);
    }
    else if (equal("float")) {
        return new_node("type_decl", ND_TYPE_DECL);
    }
    else if (equal("double")) {
        return new_node("type_decl", ND_TYPE_DECL);
    }
    else if (equal("short")) {
        return new_node("type_decl", ND_TYPE_DECL);
    }
    else if (equal("long")) {
        return new_node("type_decl", ND_TYPE_DECL);
    }
    else if (match("struct")){
        // Node *node = struct_specifier();
        struct_specifier();
        /**
         * TODO: struct type
        */
        return new_node("type_decl", ND_TYPE_DECL);
    }
    return NULL;
}

/* 6.struct-specifier −> struct [ID] [ { struct-declaration-list } ] */
/* return node type: single struct node */
static Node *struct_specifier() {
    expect("struct");
    expectType(IDENTIFIER);
    if (equal("{")) {
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
/* return node type: single function_decl node */
static Node *function_declaration() {
    Node *node = NULL;
    Node *type_decl = NULL;
    Token *id = NULL;
    Node *params = NULL;
    Node *body = NULL;

    type_decl = type_specifier();
    if (type_decl == NULL) {
        return NULL;
    }
    if (match_type(IDENTIFIER)) {
        id = &(*current_token);
        next_token();
        if (match("(")) {
            next_token();
            params = parameters();
            if (params != NULL) {
                params->is_list = true;
            }
            if (match(")")) {
                next_token();
            }
            else return NULL;
        }
        else return NULL;
    }
    body = compound_statement();
    Node *func = new_node("FunctionDeclaration", ND_FUNC_DECL);
    func->id = id;
    func->params = params;
    func->body = body;
    return func;

    // expectType(IDENTIFIER);
    // expect("(");
    // parameters();
    // expect(")");
    // compound_statement();
}

/* 9.parameters −> parameter-list | void */
/* return node type: params [] */
static Node *parameters() {
    // Node *params = new_node("FunctionParams", ND_FUNPARAMS);
    if (!equal("void")) {
        return parameter_list();
    }
    return NULL;
}

/* 10.parameter-list -> parameter { , parameter } */
static Node *parameter_list() {
    Node *head = new_node("FunctionParam", ND_PARAM);
    Node *p = head;
    Node *param = parameter();
    if (param == NULL) {
        return NULL;
    }
    else {
        head->next = param;
        p = head->next;
    }
    while (equal(",")) {
        Node *new_param = parameter();
        if (new_param == NULL) {
            printf("Error: missing function param;");
            exit(0);
        }
        p->next = new_param;
        p = p->next;
    }
    return head->next;
}

/* 11.parameter −> type-specifier ID [ “[” “]” ] */
static Node *parameter() {
    Node *type_decl = type_specifier();
    Token *id = NULL;
    if (match_type(IDENTIFIER)) {
        Token *id = &(*current_token);
        next_token();
    }
    Node *param = new_node("FunctionParam", ND_PARAM);
    param->id = id;
    if (equal("[")) {
        expect("]");
    }
    return param;
}

/* 12.compound-statement −> { [local-declarations] [statement-list] } */
static Node *compound_statement() {
    Node *block_stmt = new_node("BlockStmt", ND_BLOCK);
    block_stmt->is_list = true;
    expect("{");
    Node *decl = local_declarations();
    Node *stmt = statement_list();
    expect("}");
    if (decl->body == NULL) {
        block_stmt->body = stmt;
        return block_stmt;
    }
    else {
        decl->next = stmt;
        block_stmt->body = decl;
    }
    return block_stmt;
}

/* 13.local-declarations −> { variable-declaration } */
/**
 * TODO: 将decls串联起来并返回
*/
static Node *local_declarations() {
    Node *decl_list = new_node("DeclarationList", ND_DECL_LIST);
    Node *p = decl_list->body;
    for(;;) {
        Node *var_decl = variable_declaration();
        if (var_decl == NULL) {
            break;
        }
        if (decl_list->body == NULL) {
            decl_list->body = var_decl;
            p = decl_list->body;
            continue;
        }
        p->next = var_decl;
        p = p->next;
    }
    decl_list->is_list = true;
    return decl_list;
}

/* 14.statement-list −> { statement } */
static Node *statement_list() {
    Node *stmt_list = new_node("StatementList", ND_STMT_LIST);
    Node *p = stmt_list->body;
    for(;;) {
        Node *stmt = statement();
        if (stmt == NULL) {
            break;
        }
        if (stmt_list->body == NULL) {
            stmt_list->body = stmt;
            p = stmt_list->body;
            if (match("}")) {
                break;
            }
            continue;
        }
        p->next = stmt;
        p = p->next;
        if (match("}")) {
            break;
        }
    }
    stmt_list->is_list = true;
    return stmt_list;
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
    else if (match("while") || match("for")) {
        return iteration_statement();
    }
    else if (match("return") || match("break") || match("goto")) {
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
    expect(";");
    Node *exprStmt = new_node("ExprStmt", ND_EXPR_STMT);
    exprStmt->body = expr;
    return exprStmt;
}

/**
 * 17.selection-statement −> if ( expression ) statement [ else statement ]
 * | switch ( expression ) statement
*/
static Node *selection_statement() {
    if (equal("if")) {
        expect("(");
        Node *test = expression();
        if (test == NULL) {
            printf("if statement must have condition;");
            exit(0);
        }
        expect(")");
        Node *body = statement();
        Node *alternative = NULL;
        if (equal("else")) {
            alternative = statement();
        }
        Node *if_stmt = new_node("IfStmt", ND_IF);
        if_stmt->test = test;
        if_stmt->body = body;
        if_stmt->alternative = alternative;
        return if_stmt;
    }
    else if (equal("switch")) {
        expect("(");
        Node *test = expression();
        expect(")");
        Node *body = statement();
        Node *switch_stmt = new_node("SwitchStmt", ND_SWITCH);
        switch_stmt->test = test;
        switch_stmt->body = body;
        return switch_stmt;
    }
}

/**
 * 18.iteration-statement −> while ( expression ) statement
 * | for ( [expression ] ; [expression ] ; [expression] ) statement
*/
static Node *iteration_statement() {
    if (equal("while")) {
        expect("(");
        Node *test = expression();
        if (test == NULL) {
            printf("while statement must have condition expression");
        }
        expect(")");
        Node *body = statement();
        Node *while_stmt = new_node("WhileStmt", ND_WHILE);
        while_stmt->test = test;
        while_stmt->body = body;
        return while_stmt;
    }
    else if (equal("for")) {
        expect("(");
        Node *init = expression();
        expect(";");
        Node *test = expression();
        expect(";");
        Node *update = expression();
        Node *body = statement();
        Node *for_stmt = new_node("ForStmt", ND_FOR);
        for_stmt->init = init;
        for_stmt->test = test;
        for_stmt->body = body;
        return for_stmt;
    }
}

/**
 * 19.jump-statement −> return [ expression] ;
 * | break ; 
 * | goto ID ;
*/
static Node *jump_statement() {
    if (equal("return")) {
        Node *return_value = expression();
        expect(";");
        Node *return_stmt = new_node("ReturnStmt", ND_RETURN);
        return_stmt->body = return_value;
        return return_stmt;
    }
    else if (equal("break")) {
        expect(";");
        return new_node("BreakStmt", ND_BREAK);
    }
    else {
        expect("goto");
        expectType(IDENTIFIER);
        expect(";");
        return NULL;
    }
}

/**
 * 20.labeled-statement −> ID : statement
 * | case conditional-expression : statement
 * | default : statement
*/
static Node *labeled_statement() {
    if (equal("case")) {
        conditional_expression();
        expect(":");
        statement();
    }
    else if (equal("default")) {
        expect(":");
        statement();
    }
    else {
        expectType(IDENTIFIER);
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
static Node *expression() {
    Token *token_bak = &(*current_token);
    Node *expr = assignment_expression();
    if (expr == NULL) {
        current_token = token_bak;
        expr = conditional_expression();
    }
    return expr;
}

/**
 * 22.assignment-expression −> variable = expression
 * TODO: += -= *= /=
*/
static Node *assignment_expression() {
    Node *left = variable();
    if (left == NULL) {
        return NULL;
    }
    if (match("=")) {
        Token *op = &(*current_token);
        next_token();
        Node *right = expression();
        if (right != NULL) {
            Node *assignmentExpr = new_node("AssignmentExpr", ND_ASSIGN_EXPR);
            assignmentExpr->lhs = left;
            assignmentExpr->rhs = right;
            assignmentExpr->op = op;
            return assignmentExpr;
        }
    }
    return NULL;
    // expect("=");
    // expression();
}

/* 23.variable −> ID [ “[” NUM “]” | . ID ] */
/**
 * TODO: 补充完整
*/
static Node *variable() {
    if (match_type(IDENTIFIER)) {
        next_token();
        if (equal("[")) {
            expectType(NUMBER);
            expect("]");
        }
        if (equal(".")) {
            expectType(IDENTIFIER);
        }
        return new_node("Identifier", ND_IDENT);
    }
    return NULL;

    // expectType(IDENTIFIER);
    // if (equal("[")) {
    //     expectType(NUMBER);
    //     expect("]");
    // }
    // if (equal(".")) {
    //     expectType(IDENTIFIER);
    // }
    // return new_node("Identifier", ND_IDENT);
}

/**
 * 24.conditional-expression −> additive-expression [relation-operator additive-expression ]
 * 25.relation-operator −> <= | < | > | >= | != | ==
*/
static Node *conditional_expression() {
    Node *left = additive_expression();
    Node *right = NULL;
    if (left == NULL) {
        return NULL;
    }
    if (match("!=")
        || match("==")
        || match(">")
        || match(">=")
        || match("<")
        || match("<=")
    ) {
        Token *op = &(*current_token);
        next_token();
        right = additive_expression();
        if (right == NULL) {
            return NULL;
        }
        else {
            Node *node = new_node("BinaryExpr", ND_BINARY_EXPR);
            node->lhs = left;
            node->rhs = right;
            node->op = op;
            return node;
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
    // Node *expr = NULL;
    Node *left = multiplicative_expression();
    if (left == NULL) {
        return NULL;
    }
    while (match("+") || match("-")) {
        Token *op = &(*current_token);
        next_token();
        Node *right = multiplicative_expression();
        if (right != NULL) {
            Node *left_bak = left;
            left = new_node("binaryExpr", ND_BINARY_EXPR);
            left->lhs = left_bak;
            left->rhs = right;
            left->op = op;
        }
    }
    return left;
}

/**
 * 28.multiplicative-expression −> primary-expression {mul-operator primary-expression}
 * 29.mul-operator −> * | /
 * TODO: fix
 * 如何处理连加 / 连乘
*/
static Node *multiplicative_expression() {
    // Node *expr = new_node("binaryExpr", ND_BINARY_EXPR);
    Node *left = primary_expression();
    if (left == NULL) {
        return NULL;
    }
    while (match("*") || match("/")) {
        Token *op = &(*current_token);
        next_token();
        Node *right = primary_expression();
        if (right != NULL) {
            Node *left_bak = left;
            left = new_node("binaryExpr", ND_BINARY_EXPR);
            left->lhs = left_bak;
            left->rhs = right;
            left->op = op;
        }
    }
    return left;
    /**
     * a * b * c 
     * a left b right
     * new node (a * b) binary expr
     * (a * b) left c right
     * new node (a * b) * c binary expr
    */
}

/**
 * 30.primary-expression −> variable | NUM | ( expression ) | call-function
*/
static Node *primary_expression() {
    if (current_token && current_token->next && strcmp(current_token->next->value, "(") == 0) {
        return call_function();
    }
    if (match_type(IDENTIFIER)) {
        return variable();
    }
    else if (equalType(NUMBER)) {
        return new_node("Identifier", ND_NUM);
    }
    else if (equal("(")) {
        Node *expr = expression();
        expect(")");
        return expr;
    }
    else {
        return call_function();
    }
}

/**
 * 31.call-function −> ID ( [ argument-list ] )
*/
static Node *call_function() {
    if (match_type(IDENTIFIER)) {
        next_token();
        if (match("(")) {
            next_token();
            Node *params = argument_list();
            if (match(")")) {
                next_token();
                Node *funcall = new_node("CallExpr", ND_FUNCALL);
                funcall->params = params;
                return funcall;
            }
        }
    }
    return NULL;
    Node *node = new_node("CallExpr", ND_FUNCALL);
    expectType(IDENTIFIER);
    expect("(");
    Node *params = argument_list();
    node->body = params;
    expect(")");
    return node;
}

/**
 * 32.argument-list −> expression { , expression }
*/
static Node *argument_list() {
    Node *expr = expression();
    if (expr == NULL) {
        return NULL;
    }
    Node *p = expr;
    while (equal(",")) {
        Node *next_node = expression();
        if (next_node == NULL) {
            break;
        }
        p->next = next_node;
        p = p->next;
    }
    return expr;
}