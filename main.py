# -*- coding: utf-8 -*-
from ctypes import *
dll = CDLL('cmake-build-debug/libcpython_so.dylib')

tokenize = dll.tokenize
tokenize.argtypes = [c_char_p, c_int]
tokenize.restype = c_char_p

predict = dll.predict
predict.argtypes = [c_char_p]
predict.restype = c_char_p

v = "with a.b as";
print tokenize(v, 0)
print tokenize(v, 1)
print predict(v)



