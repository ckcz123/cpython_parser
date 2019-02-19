//
// Created by chenzhang on 08/01/2019.
//

#include "Python.h"
#include "grammar.h"
#include "tokenizer.h"
#include "errcode.h"
#include "main.h"
#include "vec.h"

const char* END_MARKER = "__END_MARKER_FOR_PARSER__";

extern grammar _PyParser_Grammar;

char* token2chars[] = {
    "", // "ENDMARKER",
    "<NAME>", // "NAME",
    "<num>", // "NUMBER",
    "<str>", // "STRING",
    "<ENTER>", // "NEWLINE", // --- Use <ENTER>
    "<IND>", // "INDENT", // --- Use <IND>
    "<UNIND>", // "DEDENT", // --- Use <UNIND>
    "(", // "LPAR",
    ")", // "RPAR",
    "[", // "LSQB",
    "]", // "RSQB",
    ":", // "COLON",
    ",", // "COMMA",
    ";", // "SEMI",
    "+", // "PLUS",
    "-", // "MINUS",
    "*", // "STAR",
    "/", // "SLASH",
    "|", // "VBAR",
    "&", // "AMPER",
    "<", // "LESS",
    ">", // "GREATER",
    "=", // "EQUAL",
    ".", // "DOT",
    "%", // "PERCENT",
    "`", // "BACKQUOTE",
    "{", // "LBRACE",
    "}", // "RBRACE",
    "==", // "EQEQUAL",
    "!=", // "NOTEQUAL",
    "<=", // "LESSEQUAL",
    ">=", // "GREATEREQUAL",
    "~", // "TILDE",
    "^", // "CIRCUMFLEX",
    "<<", // "LEFTSHIFT",
    ">>", // "RIGHTSHIFT",
    "**", // "DOUBLESTAR",
    "+=", // "PLUSEQUAL",
    "-=", // "MINEQUAL",
    "*=", // "STAREQUAL",
    "/=", // "SLASHEQUAL",
    "%=", // "PERCENTEQUAL",
    "&=", // "AMPEREQUAL",
    "|=", // "VBAREQUAL",
    "^=", // "CIRCUMFLEXEQUAL",
    "<<=", // "LEFTSHIFTEQUAL",
    ">>=", // "RIGHTSHIFTEQUAL",
    "**=", // "DOUBLESTAREQUAL",
    "//", // "DOUBLESLASH",
    "//=", // "DOUBLESLASHEQUAL",
    "@", // "AT",
    /* This table must match the #defines in token.h! */
    "<OP>", // "OP",
    "<ERROR>", // "<ERRORTOKEN>",
    "<N_TOKENS>", // "<N_TOKENS>"
};

int split_type_and_str(char* s, char** output) {
    *output = malloc(strlen(s));
    int type;
    sscanf(s, "%d %s", &type, *output);
    free(s);
    return type;
}

int get_type_from_token(char* token) {
    for (int i = 2; i <= 50; ++i) {
        if (strcmp(token, token2chars[i]) == 0)
            return i;
    }
    return 1;
}

static vec_str_t* add_token(struct tok_state *tok) {

    vec_str_t* vec_str = (vec_str_t*)malloc(sizeof(vec_str_t));
    vec_init(vec_str);

    int started = 0;
    for (;;) {
        char *a, *b;
        int type;
        size_t len;
        char *str;

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
            if (tok->indent) {
                tok->pendin = -tok->indent;
                tok->indent = 0;
            }
        } else
            started = 1;

        if (type == ENDMARKER) {
            break;
        }

        // ------ 特殊处理<str>和<float>
        int need_free = 0;
        if (type == 2 || type == 3) {
            str = token2chars[type];
        } else {
            len = b - a; /* XXX this may compute NULL - NULL */
            str = (char *) PyObject_MALLOC(len + 1);
            need_free = 1;
            if (str == NULL) {
                fprintf(stderr, "no mem for next token\n");
                break;
            }
            if (len > 0)
                strncpy(str, a, len);
            str[len] = '\0';
        }

        // --- 判定 ---
        if (strcmp(str, END_MARKER) == 0) {
            PyObject_Free(str);
            break;
        }

        if (*str == '\0') {
            PyObject_Free(str);
            need_free = 0;
            str = "__EMPTY__";
        }

        char *v = malloc(strlen(str) + 10);
        sprintf(v, "%d %s", type, str);
        if (need_free)
            PyObject_Free(str);

        vec_push(vec_str, v);
    }

    PyTokenizer_Free(tok);
    return vec_str;
}

vec_str_t* _tokenize(const char* code, int add_endmarker) {
    size_t len = strlen(code);
    char *str;
    if (add_endmarker) {
        str = (char *) PyObject_Malloc(len + strlen(END_MARKER) + 2);
        strcpy(str, code);
        char last = code[len - 1];
        // 判定最后一个字符是否是空格/换行，如果不是则需要加一个空格
        if (last != '\n' && last != ' ' && last != '\t') {
            strcat(str, " ");
        }
        // 增加结束符
        strcat(str, END_MARKER);
    } else {
        str = (char *) PyObject_Malloc(len + 1);
        strcpy(str, code);
    }

    struct tok_state *tok;
    if ((tok = PyTokenizer_FromString(str, 1)) == NULL) {
        fprintf(stderr, "ERROR in PyTokenizer_FromString\n");
        PyObject_Free(str);
        return NULL;
    }

    // parse...
    return add_token(tok);
}

char* tokenize(const char* code, int add_endmarker) {
    vec_str_t* vec_str = _tokenize(code, add_endmarker);

    // Calculate total size
    size_t totalLen = 0;
    for (int i = 0; i < vec_str->length; i++) {
        char* s = vec_str->data[i];
        totalLen += 2*strlen(s) + 1;
    }

    char* ans = malloc(totalLen);
    ans[0]='\0';
    int len = 0;
    for (int i = 0; i < vec_str->length; i++) {
        char *s = vec_str->data[i], *str;
        int type = split_type_and_str(s, &str);
        char* output = strcmp(str, "__EMPTY__") == 0 ? token2chars[type]: str;
        size_t olen = strlen(output);
        strcpy(ans+len, output);
        len += olen;
        ans[len++] = ' ';
        ans[len] = '\0';
        free(str);
    }
    vec_deinit(vec_str);
    free(vec_str);
    // printf("Allocated address: %p\n", (void*)ans);
    return ans;
}

static int _isidentifier(int type, const char* token) {
    if (type != NAME) return 0;

    grammar* g = &_PyParser_Grammar;
    int n = g->g_ll.ll_nlabels;

    const char *s = token;
    label *l = g->g_ll.ll_label;
    int i;
    // Check if it's a keyword
    for (i = n; i > 0; i--, l++) {
        if (l->lb_type != NAME || l->lb_str == NULL ||
            l->lb_str[0] != s[0] ||
            strcmp(l->lb_str, s) != 0)
            continue;
        return 0;
    }
    return 1;
}

int isidentifier(const char* token) {
    vec_str_t* vec_str = _tokenize(token, 1);
    int is = 1;
    for (int i = 0; i < vec_str->length; ++i) {
        char *s = vec_str->data[i], *str;
        int type = split_type_and_str(s, &str);
        if (i > 0 || !_isidentifier(type, str))
            is = 0;
        free(str);
    }
    vec_deinit(vec_str);
    free(vec_str);
    return is;
}

char* _get_token_info(int index) {
    label* l = _PyParser_Grammar.g_ll.ll_label+index;
    int type = l->lb_type;
    char* str = l->lb_str;
    return type >= NT_OFFSET ? NULL : str ? str : token2chars[type];
}
