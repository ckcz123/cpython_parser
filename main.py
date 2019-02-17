# -*- coding: utf-8 -*-
from ctypes import *
dll = CDLL('libcpython.so')

tokenize = dll.tokenize
tokenize.argtypes = [c_char_p, c_int]
tokenize.restype = c_char_p

predict = dll.predict
predict.argtypes = [c_char_p]
predict.restype = c_char_p

def test(code):
    print tokenize(code, 0)
    print "--------------"
    print predict(code)
    print "--------------"
    print tokenize(code, 1)
    print "-- -- -- -- -- -- -- --"

test("with a.b, c as d:")
test("for a, b in c.d:")
test("[a for b in range(c")

