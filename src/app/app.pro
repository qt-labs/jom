TEMPLATE = app
DESTDIR = ../../bin
QT = core
CONFIG += console debug_and_release build_all
TARGET = jom
DEPENDPATH += .

build_pass:CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(QMAKE_CXXFLAGS_RELEASE, -MT) {
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:msvcrt
}

SOURCES = main.cpp
include(app.pri)
