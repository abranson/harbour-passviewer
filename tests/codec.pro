TEMPLATE = app
TARGET = tst_barcodecodec
QT = core gui quick testlib
CONFIG += testcase console link_pkgconfig
PKGCONFIG += zxing
QMAKE_CXXFLAGS += -std=c++17
SOURCES += tst_barcodecodec.cpp ../src/barcodecodec.cpp
HEADERS += ../src/barcodecodec.h
SOURCES += ../src/barcodeimageprovider.cpp
HEADERS += ../src/barcodeimageprovider.h
