TEMPLATE = app
DESTDIR = ../../bin
QT = core
CONFIG += console debug_and_release build_all
TARGET = jom
DEPENDPATH += .

build_pass:CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

SOURCES = main.cpp
include(app.pri)
