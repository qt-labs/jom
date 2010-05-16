TEMPLATE = app
DEPENDPATH += . ../src/jomlib
INCLUDEPATH += ../src/jomlib
CONFIG += debug
QT += testlib

build_pass:CONFIG(debug, debug|release) {
    LIBS += ../lib/jomlibd.lib
}
build_pass:CONFIG(release, debug|release) {
    LIBS += ../lib/jomlib.lib
}

HEADERS += tests.h
SOURCES += tests.cpp

