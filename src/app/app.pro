TEMPLATE = app
DESTDIR = ../../bin
QT = core
CONFIG += console debug_and_release build_all
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
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

!CONFIG(static) {
    !build_pass:warning("You're building jom with a shared Qt.")
    LIBS += user32.lib
}

