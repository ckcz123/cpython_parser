//
// Created by chenzhang on 08/01/2019.
//

#include "Python.h"
#include "grammar.h"
#include "tokenizer.h"
#include "errcode.h"
#include "main.h"

const char* END_MARKER = "__END_MARKER_FOR_PARSER__";

extern grammar _PyParser_Grammar;

char* token2chars[] = {
    "", // "ENDMARKER",
    "<NAME>", // "NAME",
    "<NUMBER>", // "NUMBER",
    "<STRING>", // "STRING",
    "<NEWLINE>", // "NEWLINE",
    "<INDENT>", // "INDENT",
    "<DEDENT>", // "DEDENT",
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

static PyObject* add_token(struct tok_state *tok) {
    PyObject* obj = PyList_New(0);

    int started = 0;
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

        if (type == ENDMARKER) {
            break;
        }

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

        // Add ---
        // type, str, lineno, col_offset

        PyObject* tuple = PyTuple_New(4);
        PyTuple_SetItem(tuple, 0, PyLong_FromLong(type));
        PyTuple_SetItem(tuple, 1, PyString_FromString(str));
        PyTuple_SetItem(tuple, 2, PyLong_FromLong(tok->lineno));
        PyTuple_SetItem(tuple, 3, PyLong_FromLong(col_offset));

        PyList_Append(obj, tuple);
    }

    PyObject_Free(tok);
    return obj;
}

PyObject* _tokenize(const char* code, int add_endmarker) {

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
    PyObject* list = _tokenize(code, add_endmarker);

    PyObject* ans = PyString_FromString("");
    ssize_t size = PyList_Size(list);
    for (ssize_t i = 0; i < size; ++i) {
        PyObject* tuple = PyList_GetItem(list, i);
        long type = PyLong_AsLong(PyTuple_GetItem(tuple, 0));
        char* str = PyString_AsString(PyTuple_GetItem(tuple, 1));
        PyObject* newstr = PyString_FromFormat("%ld\t%s\t%s\n",
            type, _PyParser_TokenNames[type], *str=='\0'?token2chars[type]:str);
        PyString_ConcatAndDel(&ans, newstr);
    }
    Py_DecRef(list);

    return PyString_AsString(ans);
}

PyObject* _get_token_info(int index) {
    label* l = _PyParser_Grammar.g_ll.ll_label+index;
    int type = l->lb_type;
    char* str = l->lb_str;
    if (type >= NT_OFFSET) return PyString_FromString("");
    return PyString_FromFormat("%d\t%s\t%s\n", type, _PyParser_TokenNames[type], str?str:token2chars[type]);
}
