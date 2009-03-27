TEMPLATE = app
QT = core
CONFIG += console debug_and_release build_all
TARGET = mext
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ../../bin

build_pass:CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

SOURCES += main.cpp \
           consoleoutprocess.cpp
HEADERS += consoleoutprocess.h
