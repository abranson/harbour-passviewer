TEMPLATE = app
TARGET = tst_passimporter
QT = core concurrent testlib
CONFIG += testcase console c++11
SOURCES += tst_passimporter.cpp ../src/passimporter.cpp ../src/homescanner.cpp ../src/zipfile.cpp
HEADERS += ../src/passimporter.h ../src/homescanner.h ../src/zipfile.h
LIBS += -lz -lbz2 -llzma
