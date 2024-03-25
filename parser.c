#include "include/mcc.h"

/**
 * grammar in EBNF
 * 1.program -> declaration-list
 * 2.declaration-list −> declaration { declaration }
 * 3.declaration -> variable-declaration | function-declaration
 * 4.variable-declaration -> type-specifier variable-declarator { , variable-declarator } ;
 * n.variable-declarator -> ID [ "[" NUM "]" ] [ = expression ]
 * 5.type-specifier −> int | void | struct-specifier
 * 6.struct-specifier −> struct [ID] [ { struct-declaration-list} ]
 * 7.struct-declaration-list ->
 * 8.function-declaration -> type-specifier ID ( parameters ) function-body
 * n.function-body -> { [local-declarations] [statement-list] }
 * 9.parameters −> parameter-list | void | ε
 * 10.parameter-list -> parameter { , parameter }
 * 11.parameter −> type-specifier ID [ “[” “]” ]
 * 12.compound-statement −> { [statement-list] }
 * 13.local-declarations −> { variable-declaration }
 * 14.statement-list −> { statement }
 * 15.statement −> compound-statement
 * | expression-statement
 * | selection-statement 
 * | labeled-statement
 * | iteration-statement 
 * | jump-statement
 * 16.expression-statement −> [ expression ] ; | ε
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

static void next_token();
static void expect(char *str);
static void expect_type(int type);
static bool match(char *str);
static bool match_type(int type);


static Node *program();
static Node *declaration_list();
static Node *declaration();
static Node *variable_declaration();
static Node *variable_declarator();
static Node *type_specifier();
static Node *struct_specifier();
static Node *struct_declaration_list();
static Node *function_declaration();
static Vector *parameters();
static Vector *parameter_list();
static Node *parameter();
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
static Node *variable();
static Node *conditional_expression();
static Node *additive_expression();
static Node *multiplicative_expression();
static Node *primary_expression();
static Node *call_function();
static Vector *argument_list();
static Node *constant();
static Node *function_body();

void next_token () {
    current_token = current_token->next;
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

static void expect_type(int type) {
    if (current_token->type == type) {
        printf("parse success: %s\n", current_token->value);
        current_token = current_token->next;
        return;
    }
    printf("expect %d, current token: %d\t%s\n", type, current_token->id, current_token->value);
    exit(0);
}

/* 1.program -> declaration-list */
static Node *program() {
    Node *program = declaration_list();
    return program;
}

/* 2.declaration-list −> declaration { declaration } */
static Node *declaration_list() {
    Node *decl_list = new_node("DeclarationList", ND_DECL_LIST);
    while (!match_type(TK_EOF)) {
        Node *node = declaration();
        if (node == NULL) {
            break;
        }
        vec_push(decl_list->decls, node);
    }
    return decl_list;
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
 * 4.variable-declaration -> type-specifier variable-declarator { , variable-declarator } ;
 * variable-declarator -> ID [ "[" NUM "]" ] [ = expression ]
*/
static Node *variable_declaration() {
    /* type_specifier */
    Token *tok_bak = &(*current_token);
    Node *type_specifier_node = type_specifier();
    if (type_specifier_node == NULL) {
        current_token = tok_bak;
        return NULL;
    }
    Node *node = new_node("VariableDeclaration", ND_VAR_DECL);
    node->decl_type = type_specifier_node->decl_type;
    Node *var_declarator = variable_declarator();
    if (var_declarator == NULL) {
        return NULL;
    }

    var_declarator->decl_type = node->decl_type;

    vec_push(node->declarators, var_declarator);

    while (match(",")) {
        next_token();
        Node *new_declarator = variable_declarator();

        new_declarator->decl_type = node->decl_type;

        vec_push(node->declarators, new_declarator);
    }
    /* ; */
    expect(";");
    return node;
}

/**
 * variable-declarator -> ID [ "[" NUM "]" ] [ = expression ]
*/
static Node *variable_declarator() {
    Token *id = NULL;
    Node *node = new_node("VariableDeclarator", ND_VAR_DECLARATOR);
    /* ID */
    if (match_type(IDENTIFIER)) {
        id = &(*current_token);
        node->id = id;
        next_token();
        if (match("[")) {
            next_token();
            node->is_array = true;
            if (match_type(NUMBER)) {
                // node->len = current_token->value;
                next_token();
                if (match("]")) {
                    next_token();
                }
            }
        }
        if (match("=")) {
            next_token();
            Node *init = expression();
            node->init = init;
        }
        return node;
    }
    else {
        return NULL;
    }
}

/* 5.type-specifier −> int | void | struct-specifier */
static Node *type_specifier() {
    Token *tok = &(*current_token);
    /* int | char | void  */
    if (match("int")) {
        next_token();
        Node *node = new_node("type_decl", ND_TYPE_DECL);
        node->decl_type = INT;
        return node;
    }
    else if (match("char")) {
        next_token();
        Node *node = new_node("type_decl", ND_TYPE_DECL);
        node->decl_type = CHAR;
        return node;
    }
    else if (match("void")) {
        next_token();
        Node *node = new_node("type_decl", ND_TYPE_DECL);
        node->decl_type = VOID;
        return node;
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
static Node *function_declaration() {
    Node *node = NULL;
    Node *type_decl = NULL;
    Token *id = NULL;
    Vector *params = NULL;
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
    if (params != NULL)
        func->params = params;
    func->body = body;
    return func;
}

/* 9.parameters −> parameter-list | void | ε */
static Vector *parameters() {
    if (match(")")) {
        return NULL;
    }
    if (match("void")) {
        next_token();
        return NULL;
    }
    return parameter_list();
}

/* 10.parameter-list -> parameter { , parameter } */
static Vector *parameter_list() {
    Vector *params = new_vec();
    Node *param = parameter();
    if (param == NULL) {
        return NULL;
    }
    else {
        vec_push(params, param);
    }
    while (match(",")) {
        next_token();
        Node *new_param = parameter();
        if (new_param == NULL) {
            printf("Error: missing function param;");
            exit(0);
        }
        vec_push(params, new_param);
    }
    return params;
}

/* 11.parameter −> type-specifier ID [ “[” “]” ] */
static Node *parameter() {
    Node *type_decl = type_specifier();
    Token *id = NULL;
    if (match_type(IDENTIFIER)) {
        id = &(*current_token);
        next_token();
    }
    Node *param = new_node("FunctionParam", ND_FUNC_PARAM);
    param->id = id;
    if (match("[")) {
        next_token();
        expect("]");
    }
    return param;
}

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
        return NULL;
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
    else if (match("while") || match("for")) {
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
        next_token();
        expect("(");
        Node *test = expression();
        if (test == NULL) {
            printf("if statement must have condition;");
            exit(0);
        }
        expect(")");
        Node *consequent = statement();
        // first step, get if_stmt && if_stmt's test && if_stmt's consequent
        Node *if_stmt = new_node("IfStmt", ND_IF_STMT);
        if_stmt->test = test;
        if_stmt->consequent = consequent;

        // second step, link alternative to if_stmt's end
        Node *p = if_stmt->alternative; // p 始终指向if_stmt->alternative链的末尾
        while (match("else")) {
            next_token();
            if (match("if")) {
                next_token();
                expect("(");
                Node *new_test = expression();
                expect(")");
                Node *new_consequent = statement();
                Node *new_if_stmt = new_node("IfStmt", ND_IF_STMT);
                new_if_stmt->test = new_test;
                new_if_stmt->consequent = new_consequent;
                
                // 此时为第一个新alternative节点， p 还没初始化，先初始化p
                if(p == NULL) {
                    if_stmt->alternative = new_if_stmt;
                    p = if_stmt->alternative;
                    continue;
                }

                p->alternative = new_if_stmt;
                p = p->alternative;
            }
            else {
                // else if 全部结束，最后一个else节点
                Node *alternative = statement();
                if (p == NULL) {
                    if_stmt->alternative = alternative;
                    break;
                }
                else {
                    p->alternative = alternative;
                    break;
                }
            }
        }
        return if_stmt;
    }
    else if (match("switch")) {
        next_token();
        expect("(");
        Node *discriminant = expression();
        expect(")");
        expect("{");
        Node *switch_stmt = new_node("SwitchStmt", ND_SWITCH_STMT);
        while (!match("}")) {
            Node *case_stmt = labeled_statement();
            if (case_stmt != NULL) {
                vec_push(switch_stmt->cases, case_stmt);
            }
        }
        expect("}");
        switch_stmt->discriminant = discriminant;
        return switch_stmt;
    }
}

/**
 * 18.iteration-statement −> while ( expression ) statement
 * | for ( [expression ] ; [expression ] ; [expression] ) statement
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
 * 19.jump-statement −> return [ expression] ;
 * | break ; 
 * | goto ID ;
*/
static Node *jump_statement() {
    if (match("return")) {
        next_token();
        Node *return_value = expression();
        expect(";");
        Node *return_stmt = new_node("ReturnStmt", ND_RETURN_STMT);
        return_stmt->body = return_value;
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
        Node *test = primary_expression();
        expect(":");
        node->consequent = statement();
        return node;
    }
    else if (match("default")) {
        next_token();
        Node *node = new_node("Case", ND_CASE);
        node->test = NULL;
        expect(":");
        node->consequent = statement();
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
        Token *id = &(*current_token);
        next_token();
        if (match("[")) {
            next_token();
            expect_type(NUMBER);
            expect("]");
        }
        if (match(".")) {
            next_token();
            expect_type(IDENTIFIER);
        }
        Node *node = new_node("Identifier", ND_IDENT);
        node->id = id;
        return node;
    }
    return NULL;
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
 * 30.primary-expression −> variable | constant | ( expression ) | call-function
*/
static Node *primary_expression() {
    if (current_token && current_token->next && strcmp(current_token->next->value, "(") == 0) {
        return call_function();
    }
    if (match_type(IDENTIFIER)) {
        return variable();
    }
    else if (match_type(NUMBER) || match_type(CHARACTER) || match_type(STRING)) {
        // Token *tok = &(*current_token);
        // next_token();
        // Node *node = new_node("Literal", ND_NUM);
        // node->tok = tok;
        return constant();
    }
    else if (match("(")) {
        next_token();
        Node *expr = expression();
        expect(")");
        return expr;
    }
    else {
        return call_function();
    }
}

/**
 * constant -> NUM | CHAR | String
*/
static Node *constant() {
    Token *tok = &(*current_token);
    if (match_type(NUMBER)) {
        next_token();
        Node *node = new_node("Constant", ND_NUM);
        node->tok = tok;
        return node;
    }
    else if (match_type(CHARACTER)) {
        next_token();
        Node *node = new_node("Constant", ND_CHAR);
        node->tok = tok;
        return node;
    }
    else if (match_type(STRING)) {
        next_token();
        Node *node = new_node("Constant", ND_STR);
        node->tok = tok;
        return node;
    }
}

/**
 * 31.call-function −> ID ( [ argument-list ] )
*/
static Node *call_function() {
    if (match_type(IDENTIFIER)) {
        Token *tok = &(*current_token);
        next_token();
        if (match("(")) {
            next_token();
            Vector *args = argument_list();
            if (match(")")) {
                next_token();
                Node *funcall = new_node("CallExpr", ND_FUNCALL);
                funcall->id = tok;
                funcall->args = args;
                return funcall;
            }
        }
    }
    return NULL;
    // Node *node = new_node("CallExpr", ND_FUNCALL);
    // expect_type(IDENTIFIER);
    // expect("(");
    // Vector *args = argument_list();
    // node->args = args;
    // expect(")");
    // return node;
}

/**
 * 32.argument-list −> expression { , expression }
*/
static Vector *argument_list() {
    Vector *args = new_vec();
    Node *expr = expression();
    if (expr == NULL) {
        return NULL;
    }
    vec_push(args, expr);
    // Node *p = expr;
    while (match(",")) {
        next_token();
        Node *new_node = expression();
        if (new_node == NULL) {
            break;
        }
        vec_push(args, new_node);
        // p->next = next_node;
        // p = p->next;
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