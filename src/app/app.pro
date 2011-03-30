TEMPLATE = app
DESTDIR = ../../bin
QT = core
CONFIG += console debug_and_release build_all
TARGET = jom
DEPENDPATH += .

build_pass:CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += ../../lib/jomlibd.lib
}

build_pass:CONFIG(release, debug|release) {
    LIBS += ../../lib/jomlib.lib
}

contains(QMAKE_CXXFLAGS_RELEASE, -MT) {
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:msvcrt
}

INCLUDEPATH += ../jomlib
HEADERS = application.h
SOURCES = main.cpp application.cpp

