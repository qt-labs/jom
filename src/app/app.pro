TEMPLATE = app
DESTDIR = ../../bin
QT = core
CONFIG += console
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
TARGET = jom
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

PROJECT_BUILD_ROOT=$$OUT_PWD/../..
include(../jomlib/use_jomlib.pri)

contains(QMAKE_CXXFLAGS_RELEASE, -MT) {
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:msvcrt
}

INCLUDEPATH += ../jomlib
HEADERS = application.h
SOURCES = main.cpp application.cpp
RESOURCES = app.qrc
