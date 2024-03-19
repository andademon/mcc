#include "include/mcc.h"

static Token *head;

int main(int argc, char *argv[])
{
    File *file = new_file("demo.c", "");
    char *str = readFile("demo.c");
    file->content = str;
    printf("%s\n", file->content);
    head = Lexer(file->content);

    printTokenList(head);
    Program *prog = parse(head);
    printProgram(prog);
    return 0;
}
