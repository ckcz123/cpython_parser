# -*- coding: utf-8 -*-
from ctypes import *
import os

class CPythonHelper:
    def __init__(self):
        dll =  CDLL(os.path.join(os.path.dirname(__file__), 'libcpython.so'))

        _tokenize = dll.tokenize
        _tokenize.argtypes = [c_char_p, c_int]
        _tokenize.restype = c_void_p

        _predict = dll.predict
        _predict.argtypes = [c_char_p]
        _predict.restype = c_void_p

        _predict2 = dll.predict2
        _predict2.argtypes = [c_char_p]
        _predict2.restype = c_void_p

        _isidentifier = dll.isidentifier
        _isidentifier.argtypes = [c_char_p]
        _isidentifier.restype = c_int

        _free = dll.freeme
        _free.argtypes = [c_void_p]
        _free.restype = None

        self.dll = dll
        self._tokenize = _tokenize
        self._predict = _predict
        self._predict2 = _predict2
        self._isidentifier = _isidentifier
        self._free = _free

    """
    Try to tokenize given code. Returns a string, separated by a single space.
    Sample input:
        [a for b in range(c
    Sample output: (add_endmarker=0)
        [ a for b in range ( c <ENTER>
    Sample input:
        if a in c["hello"][4]:
    Sample output: (add_endmarker=1)
        if a in c [ <STRING> ] [ <NUMBER> ] :
    """
    def tokenize(self, code, add_endmarker=0):
        raw = self._tokenize(code, add_endmarker)
        ans = cast(raw, c_char_p).value
        self._free(raw)
        return ans

    """
    Try to predict next token. Returns a list, <NAME> for any valid name.
    Input can be a string or a list of tokenized tokens.
    Sample input:
        "[a for b in range(c"
        OR
        ['[', 'a', 'for', '<unk>', 'in', 'range', '(', 'c']
    Sample output
        ['(', '**', '.', '[', '*', ..., 'in', 'not', '<', '>', '==', ..., 'is', 'and', 'or', 'if', '=', 'for', ',', ')']
    """
    def predict(self, code):
        if type(code) is list:
            raw = self._predict(" ".join(code))
        elif type(code) is str:
            raw = self._predict2(code)
        else:
            return []
        ans = cast(raw, c_char_p).value
        self._free(raw)
        if ans is None:
            return None
        return ans.strip().split(" ")

    """
    If this token is an identifier.
    Input should be a single token.
    """
    def isidentifier(self, token):
        return self._isidentifier(token) > 0

if __name__ == '__main__':
    helper = CPythonHelper()
    print helper.tokenize("if a in c[\"hello\"][4]:")
    print helper.predict(['[', 'a', 'for', '<unk>', 'in', 'range', '(', 'c'])
    print helper.predict("[a for b in range(")
    print helper.isidentifier("print")
    print helper.predict(["if", "a", ":", "<ENTER>"])
    print helper.tokenize("if a: # hahaha\n    b = c")
