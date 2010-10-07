TEMPLATE = app
DEPENDPATH += . ../src/jomlib
INCLUDEPATH += ../src/jomlib
CONFIG += debug_and_release
QT += testlib

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

