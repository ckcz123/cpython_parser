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
#include <string.h>
#include <stdio.h>

#define D(x) x

extern grammar _PyParser_Grammar;

const char* END_MARKER = "__END_MARKER_AND_PREDICT__";

void
__printtree(parser_state *ps);


// 返回当前自动机的状态
// 参照 parsetok.c/parsetok
parser_state* my_parsetok(struct tok_state *tok) {
    parser_state* ps;
    node* n;
    int started = 0;
    if ((ps = PyParser_New(&_PyParser_Grammar, Py_file_input)) == NULL) {
        fprintf(stderr, "no mem for new parser\n");
        PyTokenizer_Free(tok);
        return NULL;
    }
    for (;;) {
        char *a, *b;
        int type;
        size_t len;
        char *str;
        int col_offset;

        type = PyTokenizer_Get(tok, &a, &b);
        if (type == ERRORTOKEN) {
            fprintf(stderr, "ERRORTOKEN\n");
            break;
        }
        if (type == ENDMARKER && started) {
            type = NEWLINE; /* Add an extra newline */
            started = 0;
            /* Add the right number of dedent tokens,
               except if a certain flag is given --
               codeop.py uses this. */
            if (tok->indent)
            {
                tok->pendin = -tok->indent;
                tok->indent = 0;
            }
        }
        else
            started = 1;
        len = b - a; /* XXX this may compute NULL - NULL */
        str = (char *) PyObject_MALLOC(len + 1);
        if (str == NULL) {
            fprintf(stderr, "no mem for next token\n");
            break;
        }
        if (len > 0)
            strncpy(str, a, len);
        str[len] = '\0';

        // --- 判定 ---
        if (strcmp(str, END_MARKER) == 0) {
            PyObject_Free(str);
            break;
        }

        if (a >= tok->line_start)
            col_offset = a - tok->line_start;
        else
            col_offset = -1;

        int error;
        if ((error = PyParser_AddToken(ps, type, str, tok->lineno, col_offset,
                                   NULL)) != E_OK) {
            if (error != E_DONE) {
                fprintf(stderr, "ERROR in parsing...\n");
                PyObject_Free(tok);
                PyParser_Delete(ps);
                PyObject_FREE(str);
                return NULL;
            }
            break;
        }
    }

    PyObject_Free(tok);
    return ps;
}

// 实际进行预测计算
// 参照 parser.c/PyParser_AddToken
int* calculate(parser_state* ps, int* size) {
    int* ans = (int*)calloc(_PyParser_Grammar.g_ll.ll_nlabels, sizeof(int));
    // int ans[200];
    int ss = 0;
    *size = 0;
    int contains[200] = {0};

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
                ans[ss++] = i;
                *size = (*size)+1;
                /*
                if (x & (1<<7)) {
                    // 检测非终结符
                    int nt = (x>>8) + NT_OFFSET;
                    dfa *d1 = PyGrammar_FindDFA(ps->p_grammar, nt);
                    // --- 加入 firstsets 中的所有内容 ---

                }
                else {
                    // 终结符，直接加到结果里面
                    contains[i] = 1;
                    ans[*size++] = i;
                }
                 */
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


// 预测下一个结果
int* predict(const char* code, int* size) {
    *size = 0;

    // ------ 增添结束标志，前面可能需要加个空格防止冲突 ------
    int len = strlen(code);
    char str[100000];
    strcpy(str, code);
    // 判定最后一个字符是否是空格/换行，如果不是则需要加一个空格
    char last = str[len-1];
    if (last != '\n' && last != ' ' && last != '\t') {
        strcat(str, " ");
    }
    // 增加结束符
    strcat(str, END_MARKER);

    // ------ 初始化 ------
    struct tok_state *tok;
    if ((tok = PyTokenizer_FromString(str, 1)) == NULL) {
        fprintf(stderr, "ERROR in PyTokenizer_FromString\n");
        return NULL;
    }
    tok->filename = "<string>";

    // ------ 计算到当前状态 ------
    parser_state* state = my_parsetok(tok);
    if (state == NULL) {
        fprintf(stderr, "Failed....\n");
        return NULL;
    }

    // ------ 预测下个状态 ------
    __printtree(state);
    return calculate(state, size);
}
