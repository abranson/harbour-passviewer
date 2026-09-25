TEMPLATE = app
TARGET = tst_settingsstore
QT = core testlib
CONFIG += testcase console
SOURCES += tst_settingsstore.cpp ../src/settingsstore.cpp
HEADERS += ../src/settingsstore.h
