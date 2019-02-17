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

        _isidentifier = dll.isidentifier
        _isidentifier.argtypes = [c_char_p]
        _isidentifier.restype = c_int

        _free = dll.freeme
        _free.argtypes = [c_void_p]
        _free.restype = None

        self.dll = dll
        self._tokenize = _tokenize
        self._predict = _predict
        self._isidentifier = _isidentifier
        self._free = _free

    """
    Try to tokenize given code. Returns a string, separated by a single space.
    Input:
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
    Try to predict next code. Returns a list, <NAME> for any valid name.
    """
    def predict(self, code):
        raw = self._predict(code)
        ans = cast(raw, c_char_p).value
        self._free(raw)
        return ans.strip().split(" ")

    """
    If this token is an identifier.
    """
    def isidentifier(self, token):
        return self._isidentifier(token) > 0

if __name__ == '__main__':
    helper = CPythonHelper()
    print helper.tokenize("if a in c[\"hello\"][4]:")
    print helper.predict("if a in c[\"hello\"][4]:")
    print helper.isidentifier("print")
