TEMPLATE = app
TARGET = 
DEPENDPATH += . ../src/app
INCLUDEPATH += ../src/app
QT += testlib

SOURCES += tests.cpp

include(../src/app/app.pri)

