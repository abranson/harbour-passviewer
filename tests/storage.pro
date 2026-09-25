TEMPLATE = app
TARGET = tst_savedcards
QT = core gui testlib
CONFIG += testcase console c++11
SOURCES += tst_savedcards.cpp ../src/savedcards.cpp
HEADERS += ../src/savedcards.h
