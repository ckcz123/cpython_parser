//
// Created by chenzhang on 07/01/2019.
//

#include "Python.h"
#include "tokenizer.h"
#include "grammar.h"
#include "node.h"
#include "parser.h"
#include "parsetok.h"
#include "graminit.h"
#include "errcode.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

#define D(x) x

extern grammar _PyParser_Grammar;

extern void __printtree(parser_state *ps);

extern PyObject* _tokenize(const char*, int);

extern PyObject* _get_token_info(int);

static parser_state* run_to_current(PyObject* list) {
    if (list == NULL)
        return NULL;

    ssize_t len = PyList_Size(list);

    parser_state* ps;
    if ((ps = PyParser_New(&_PyParser_Grammar, Py_file_input)) == NULL) {
        fprintf(stderr, "no mem for new parser\n");
        return NULL;
    }
    for (ssize_t i = 0; i < len; ++i) {
        PyObject* tuple = PyList_GetItem(list, i);
        int type = (int)PyLong_AsLong(PyTuple_GetItem(tuple, 0));
        char* str = PyString_AsString(PyTuple_GetItem(tuple, 1));
        int lineno = (int)PyLong_AsLong(PyTuple_GetItem(tuple, 2));
        int col_offset = (int)PyLong_AsLong(PyTuple_GetItem(tuple, 3));

        int error;
        if ((error = PyParser_AddToken(ps, type, str, lineno, col_offset,
                                       NULL)) != E_OK) {
            if (error != E_DONE) {
                fprintf(stderr, "ERROR in parsing...\n");
                PyParser_Delete(ps);
                return NULL;
            }
            break;
        }
    }

    // Get PS
    D(__printtree(ps));
    return ps;
}

static int* predict_next(parser_state* ps, int* size) {
    int* ans = (int*)calloc(_PyParser_Grammar.g_ll.ll_nlabels, sizeof(int));
    *size = 0;
    int contains[256] = {0};

    for (;;) {
        // ------ 检查当前的子状态
        dfa* d = ps->p_stack.s_top->s_dfa;
        state* s = &d->d_state[ps->p_stack.s_top->s_state];

        D(printf(" DFA '%s', state %d...\n",
                 d->d_name, ps->p_stack.s_top->s_state));
        // 从lower检查到upper
        for (int i = s->s_lower; i < s->s_upper; i++) {
            int x = s->s_accel[i - s->s_lower];
            if (x != -1) { // 可以接受
                // 如果已经被加入到结果集，则忽略
                if (contains[i] || (_PyParser_Grammar.g_ll.ll_label+i)->lb_type >= NT_OFFSET)
                    continue;
                contains[i] = 1;
                ans[*size] = i;
                *size = (*size)+1;
            }
        }

        // ------ 能否弹出？如果能则继续
        if (s->s_accept) {
            ps->p_stack.s_top++;
            if (ps->p_stack.s_top != &(ps->p_stack.s_base[MAXSTACK])) {
                continue;
            }
        }
        break;
    }

    PyParser_Delete(ps);
    return ans;
}

char* predict(const char* code) {
    parser_state* ps = run_to_current(_tokenize(code, 1));
    if (ps == NULL)
        return NULL;

    int size = 0;
    int* valid = predict_next(ps, &size);

    if (valid == NULL)
        return NULL;

    PyObject* ans = PyString_FromString("");
    for (int i = 0; i < size; ++i) {
        PyString_ConcatAndDel(&ans, _get_token_info(valid[i]));
    }

    return PyString_AsString(ans);
}