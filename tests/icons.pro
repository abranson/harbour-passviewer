TEMPLATE = app
TARGET = tst_cardicons
QT = core gui network testlib
CONFIG += testcase console c++11
SOURCES += tst_cardicons.cpp ../src/cardicons.cpp
HEADERS += ../src/cardicons.h
