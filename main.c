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
#include <stdio.h>

extern grammar _PyParser_Grammar;

int main() {
    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        printf("arena null");
        return 0;
    }
    int iflags = 0;
    perrdetail err;
    node *n = PyParser_ParseStringFlagsFilenameEx(
        "with a.b as c:\n    pass",
        // "with gfile.GFile(currdir+'/frozen_graph', 'rb') as f:\n    a = 4+5\n    pass",
        "<string>", &_PyParser_Grammar, Py_file_input, &err, &iflags);
    printf("%s\n", n->n_str);
    PyArena_Free(arena);
    return 0;
}
