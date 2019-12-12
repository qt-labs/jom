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

JOM_VERSION = $$cat(version.txt, lines)
JOM_VERSION = $$first(JOM_VERSION)
JOM_VERSION_MAJOR=$$section(JOM_VERSION, '.', 0, 0)
JOM_VERSION_MINOR=$$section(JOM_VERSION, '.', 1, 1)
JOM_VERSION_PATCH=$$section(JOM_VERSION, '.', 2, 2)
DEFINES += JOM_VERSION_MAJOR=$$JOM_VERSION_MAJOR
DEFINES += JOM_VERSION_MINOR=$$JOM_VERSION_MINOR
DEFINES += JOM_VERSION_PATCH=$$JOM_VERSION_PATCH

content = $$cat(app.rc.in, blob)
content ~= s/@\([^@]+\)@/\$\${\1}/g
content ~= s/\"/\\\"/g
write_file($$OUT_PWD/jom.rc.in, content)
rcsubst.input = $$OUT_PWD/jom.rc.in
rcsubst.output = $$OUT_PWD/jom.rc
QMAKE_SUBSTITUTES += rcsubst
RC_FILE += $$OUT_PWD/jom.rc
