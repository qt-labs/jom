TEMPLATE = app
CONFIG += debug_and_release
QT += testlib
DEPENDPATH += . ../src/jomlib
INCLUDEPATH += ../src/jomlib
DEFINES += SRCDIR=\\\"$$PWD/\\\"

build_pass:CONFIG(debug, debug|release) {
    LIBS += ../lib/jomlibd.lib
}
build_pass:CONFIG(release, debug|release) {
    LIBS += ../lib/jomlib.lib
}
contains(QMAKE_CXXFLAGS_RELEASE, -MT) {
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:msvcrt
}

HEADERS += tests.h
SOURCES += tests.cpp

