TEMPLATE = app
TARGET = tst_passsharer
QT = core gui concurrent testlib
CONFIG += testcase console link_pkgconfig
PKGCONFIG += zxing
QMAKE_CXXFLAGS += -std=c++17
SOURCES += tst_passsharer.cpp ../src/passsharer.cpp ../src/savedcards.cpp ../src/barcodecodec.cpp ../src/zipfile.cpp ../src/passimporter.cpp
HEADERS += ../src/passsharer.h ../src/savedcards.h ../src/barcodecodec.h ../src/zipfile.h ../src/passimporter.h
LIBS += -lz -lbz2 -llzma
SOURCES += ../src/homescanner.cpp
HEADERS += ../src/homescanner.h
