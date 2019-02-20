//
// Created by chenzhang on 04/01/2019.
//

#include "Python.h"
#include "grammar.h"
#include "graminit.h"
#include "node.h"
#include "token.h"
#include "parsetok.h"
#include "errcode.h"
#include "main.h"
#include <stdio.h>

void freeme(char* ptr)
{
    if (ptr != NULL) {
        // printf("Freeing memory... %p\n", (void*)ptr);
        free(ptr);
    }
}

int main() {

    char* s = tokenize("if a:# hahaha\n    b = c", 0);
    printf("%s\n", s);
    freeme(s);

    s = tokenize("for a, b in c['x'][2", 1);
    printf("%s\n", s);
    freeme(s);

    s = predict("for a , <unk> in c [ <str> ] [ <num>");
    printf("%s\n", s);
    freeme(s);

    s = predict2("for a, b in c['x'][2");
    printf("%s\n", s);
    freeme(s);

    printf("%d\n", isidentifier("print"));
    return 0;
}
