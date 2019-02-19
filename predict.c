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
#include "vec.h"
#include <string.h>
#include <stdio.h>

#define D(x)

extern grammar _PyParser_Grammar;

extern void __printtree(parser_state *ps);

extern int split_type_and_str(char* s, char** output);

extern int get_type_from_token(char* token);

extern vec_str_t* _tokenize(const char*, int);

extern char* _get_token_info(int);

static parser_state* run_to_current(vec_str_t* vec_str, const char* code) {
    int len = vec_str->length;
    parser_state* ps;
    if ((ps = PyParser_New(&_PyParser_Grammar, Py_file_input)) == NULL) {
        fprintf(stderr, "no mem for new parser\n");
        return NULL;
    }
    for (int i = 0; i < len; ++i) {
        char* s = vec_str->data[i], *str;
        int type = split_type_and_str(s, &str);
        int error;
        if ((error = PyParser_AddToken(ps, type, str, 1, 0,
                                       NULL)) != E_OK) {
            if (error != E_DONE) {
                fprintf(stderr, "ERROR in parsing: %s\n", code);
                PyParser_Delete(ps);
                return NULL;
            }
            break;
        }
    }

    // Get PS
    D(__printtree(ps));
    vec_deinit(vec_str);
    free(vec_str);
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

static char* _predict(vec_str_t* vec_str, const char* code) {
    parser_state* ps = run_to_current(vec_str, code);
    if (ps == NULL)
        return NULL;

    int size = 0;
    int* valid = predict_next(ps, &size);

    if (valid == NULL)
        return NULL;

    // Concatenate
    size_t total = 0;
    for (int i = 0; i < size; ++i) {
        char* str = _get_token_info(valid[i]);
        if (str)
            total += strlen(str) + 1;
    }

    char* ans = malloc(total);
    ans[0] = '\0';
    int now = 0;
    for (int i = 0; i < size; ++i) {
        char* str = _get_token_info(valid[i]);
        if (str) {
            size_t len = strlen(str);
            strcpy(ans+now, str);
            now += len;
            ans[now++] = ' ';
            ans[now] = '\0';
        }
    }

    free(valid);
    // printf("Allocated address: %p\n", (void*)ans);
    return ans;
}

char* predict(const char* tokens) {
    vec_str_t* vec_str = (vec_str_t*)malloc(sizeof(vec_str_t));
    vec_init(vec_str);

    char* code = strdup(tokens);

    char* start = code, * end = code;
    while (*end != '\0') {
        end++;
        if (*end == ' ') {
            *end = '\0';
            if (start != end) {
                char* str = malloc(10 + (end-start));
                sprintf(str, "%d %s", get_type_from_token(start), start);
                vec_push(vec_str, str);
            }
            end++;
            start = end;
        }
    }
    if (start != end) {
        char* str = malloc(10 + (end-start));
        sprintf(str, "%d %s", get_type_from_token(start), start);
        vec_push(vec_str, str);
    }

    free(code);

    return _predict(vec_str, code);
}

char* predict2(const char* code) {
    return _predict(_tokenize(code, 1), code);
}