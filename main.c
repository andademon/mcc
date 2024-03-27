#include "include/mcc.h"

static Token *head;

int main(int argc, char *argv[])
{
    char *str = readFile("demo.c");
    printf("%s\n", str);

    printf("---Lexer---\n");
    head = Lexer(str);
    printTokenList(head);

    printf("---Parser---\n");
    Program *prog = parse(head);
    printProgram(prog);
    puts("");

    prog = tree_to_prog(prog);
    printf("---Semantic---\n");
    sema(prog);
    puts("");

    printf("---Codegen---\n");
    codegen(prog);

    // codegen(prog);
    // codegen测试
    // 测试全局变量
    // printf("---test gvars---\n");
    // for (int i = 0;i < prog->gvars->len;i++) {
    //     codegen_test(prog->gvars->data[i]);
    // }

    // 测试局部变量
    // printf("---test lvars---\n");
    // Node *func1 = prog->funcs->data[0];
    // codegen_test_function(func1);
    return 0;
}
