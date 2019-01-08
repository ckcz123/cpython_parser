cmake_minimum_required (VERSION 3.1)

PROJECT(cpython_parser)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -fPIC")

INCLUDE_DIRECTORIES(/usr/local/Cellar/python@2/2.7.15_1/Frameworks/Python.framework/Versions/2.7/include/python2.7)

file(GLOB SOURCES *.c)

add_executable(cpython_parser ${SOURCES})
target_link_libraries(cpython_parser python2.7)

add_library(cpython_so SHARED ${SOURCES})
target_link_libraries(cpython_so python2.7)
