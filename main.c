#include "include/mcc.h"

static Token *head;
void test();

int main(int argc, char *argv[])
{
    // test();
    if (argc < 2) {
        printf("At least two command line parameters are required. Example: ./main <filename>\n");
        exit(1);
    }

    char *str = readFile(argv[1]);
    head = Lexer(str);
    Program *prog = parse(head);

    if (argc == 2) {
        printf("---Source File---\n");
        printf("%s\n", str);

        printf("---Lexer---\n"); 
        printTokenList(head);

        printf("---Parser---\n");
        printProgram(prog);
        
        printf("---Semantic---\n");
        prog = tree_to_prog(prog);
        SymbolTable *table = sema(prog);

        printf("---Codegen---\n");
        codegen(prog, table);
    }
    else {
        for (int i = 2;i < argc;i++) {
            if (!strcmp(argv[i], "-file")) {
                printf("%s\n", str);
                continue;
            }
            else if (!strcmp(argv[i], "-token")) {
                printTokenList(head);
                continue;
            }
            else if (!strcmp(argv[i], "-tree")) {
                printProgram(prog);
                continue;
            }
            else if (!strcmp(argv[i], "-code")) {
                prog = tree_to_prog(prog);
                SymbolTable *table = sema(prog);
                codegen(prog, table);
            }
            else {
                printf("unknown command line arg: %s", argv[i]);
                exit(1);
            }
        }
    }
    return 0;
}

void test() {
    // char *str = readFile("test/demo.c");
    // char *str = readFile("test/hello.c");
    // char *str = readFile("test/fibonacci.c");
    // char *str = readFile("test/gval.c");
    // char *str = readFile("test/gval-array.c");
    // char *str = readFile("test/while.c");
    // char *str = readFile("test/do-while.c");
    // char *str = readFile("test/for.c");
    // char *str = readFile("test/if-else.c");
    char *str = readFile("test/switch-case.c");
    // char *str = readFile("test/array.c");
    // char *str = readFile("test/pointer.c");

    // char *str = readFile("test/quicksort.c");

    // char *str = readFile("test/complex.c");

    // char *str = readFile("test/MultidimensionalArray.c");
    // char *str = readFile("test/goto.c");

    // printf("---Source File---\n");
    // printf("%s\n", str);

    // printf("---Lexer---\n"); 
    head = Lexer(str);
    // printTokenList(head);

    // printf("---Parser---\n");
    Program *prog = parse(head);
    // printProgram(prog);
    
    prog = tree_to_prog(prog);
    // printf("---Semantic---\n");
    SymbolTable *table = sema(prog);

    // printf("---Codegen---\n");
    codegen(prog, table);
    exit(0);
}
