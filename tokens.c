//
// Created by chenzhang on 08/01/2019.
//

#include "Python.h"
#include "grammar.h"
#include "tokenizer.h"
#include "errcode.h"
#include "main.h"

typedef struct {
    short type;
    char* name;
    char* str;
} token;

const char* END_MARKER = "__END_MARKER_FOR_PARSER__";

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

token PyParser_Tokens[] = {
    {0, "ENDMARKER", "EMPTY"},
    {-1, NULL, NULL},
    {4, "NEWLINE", "<NEWLINE>"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {0, "ENDMARKER", ""},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {50, "AT", "@"},
    {-1, NULL, NULL},
    {7, "LPAR", "("},
    {-1, NULL, NULL},
    {8, "RPAR", ")"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "def"},
    {1, "NAME", "<NAME>"},
    {-1, NULL, NULL},
    {11, "COLON", ":"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {22, "EQUAL", "="},
    {-1, NULL, NULL},
    {12, "COMMA", ","},
    {16, "STAR", "*"},
    {36, "DOUBLESTAR", "**"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {13, "SEMI", ";"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {37, "PLUSEQUAL", "+="},
    {38, "MINEQUAL", "-="},
    {39, "STAREQUAL", "*="},
    {40, "SLASHEQUAL", "/="},
    {41, "PERCENTEQUAL", "%="},
    {42, "AMPEREQUAL", "&="},
    {43, "VBAREQUAL", "|="},
    {44, "CIRCUMFLEXEQUAL", "^="},
    {45, "LEFTSHIFTEQUAL", "<<="},
    {46, "RIGHTSHIFTEQUAL", ">>="},
    {47, "DOUBLESTAREQUAL", "**="},
    {49, "DOUBLESLASHEQUAL", "//="},
    {1, "NAME", "print"},
    {35, "RIGHTSHIFT", ">>"},
    {1, "NAME", "del"},
    {-1, NULL, NULL},
    {1, "NAME", "pass"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "break"},
    {1, "NAME", "continue"},
    {1, "NAME", "return"},
    {1, "NAME", "raise"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "import"},
    {-1, NULL, NULL},
    {1, "NAME", "from"},
    {23, "DOT", "."},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "as"},
    {-1, NULL, NULL},
    {1, "NAME", "global"},
    {1, "NAME", "exec"},
    {-1, NULL, NULL},
    {1, "NAME", "in"},
    {1, "NAME", "assert"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "if"},
    {1, "NAME", "elif"},
    {1, "NAME", "else"},
    {1, "NAME", "while"},
    {1, "NAME", "for"},
    {1, "NAME", "try"},
    {-1, NULL, NULL},
    {1, "NAME", "finally"},
    {1, "NAME", "with"},
    {-1, NULL, NULL},
    {1, "NAME", "except"},
    {5, "INDENT", "<INDENT>"},
    {6, "DEDENT", "<DEDENT>"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "lambda"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "or"},
    {-1, NULL, NULL},
    {1, "NAME", "and"},
    {1, "NAME", "not"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {20, "LESS", "<"},
    {21, "GREATER", ">"},
    {28, "EQEQUAL", "=="},
    {31, "GREATEREQUAL", ">="},
    {30, "LESSEQUAL", "<="},
    {29, "NOTEQUAL", "!="},
    {29, "NOTEQUAL", "!="},
    {1, "NAME", "is"},
    {-1, NULL, NULL},
    {18, "VBAR", "|"},
    {-1, NULL, NULL},
    {33, "CIRCUMFLEX", "^"},
    {-1, NULL, NULL},
    {19, "AMPER", "&"},
    {-1, NULL, NULL},
    {34, "LEFTSHIFT", "<<"},
    {-1, NULL, NULL},
    {14, "PLUS", "+"},
    {15, "MINUS", "-"},
    {-1, NULL, NULL},
    {17, "SLASH", "/"},
    {24, "PERCENT", "%"},
    {48, "DOUBLESLASH", "//"},
    {32, "TILDE", "~"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {9, "LSQB", "["},
    {-1, NULL, NULL},
    {10, "RSQB", "]"},
    {26, "LBRACE", "{"},
    {-1, NULL, NULL},
    {27, "RBRACE", "}"},
    {25, "BACKQUOTE", "`"},
    {-1, NULL, NULL},
    {2, "NUMBER", "<NUMBER>"},
    {3, "STRING", "<STRING>"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "class"},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {-1, NULL, NULL},
    {1, "NAME", "yield"}
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

    // Concatenate to one char
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

PyObject* get_token_string(int index) {
    token* tok = &PyParser_Tokens[index];
    return PyString_FromFormat("%d\t%s\t%s\n", tok->type, tok->name, tok->str);
}
