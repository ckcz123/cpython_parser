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

// extern grammar _PyParser_Grammar;
// extern char* token2chars[];

int main() {
    printf("%s\n", predict("with a.b as x:\n\ty\nz"));
    printf("-----------\n\n");
    printf("%s\n", tokenize("with a.b as x:\n\ty\nz", 0));
    // printf("%s\n", tokenize("with a.b as", 0));

    /*
    int size;
    int* valid = predict("with a.b as", &size);
    if (valid != NULL) {
        for (int i = 0; i < size; i++) {
            label* l = _PyParser_Grammar.g_ll.ll_label + valid[i];
            printf("{%d, %s}\n", l->lb_type, l->lb_str?l->lb_str:_PyParser_TokenNames[l->lb_type]);
        }
    }
    printf("Size = %d\n", size);
    PyObject_Free(valid);
     */

    /*
    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        printf("arena null");
        return 0;
    }
    int iflags = 0;
    perrdetail err;
    node *n = PyParser_ParseStringFlagsFilenameEx(
        // "with a.b as c:\n    pass",
        "with a, b:\n __QUERY_TOKEN_FROM_PARSER__\n    pass",
        // "with gfile.GFile(currdir+'/frozen_graph', 'rb') as f:\n    a = 4+5\n    pass",
        "<string>", &_PyParser_Grammar, Py_file_input, &err, &iflags);
    printf("%s\n", n->n_str);
    PyArena_Free(arena);
     */

    return 0;
}
