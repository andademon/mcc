#include "include/mcc.h"

static Token *head;

int main(int argc, char *argv[])
{
    // char *str = readFile("test/demo.c");
    // char *str = readFile("test/hello.c");
    // char *str = readFile("test/fibonacci.c");
    // char *str = readFile("test/gval.c");
    // char *str = readFile("test/while.c");
    // char *str = readFile("test/do-while.c");
    // char *str = readFile("test/for.c");
    // char *str = readFile("test/if-else.c");
    char *str = readFile("test/switch-case.c");

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

    return 0;
}
