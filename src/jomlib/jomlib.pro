TEMPLATE = lib
TARGET = jomlib
DESTDIR = ../../lib
QT = core
CONFIG += qt staticlib debug_and_release build_all

build_pass:CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

MSYSPATH=C:\\msys
!exists($$MSYSPATH): MSYSPATH=D:\\msys
!exists($$MSYSPATH) {
    !build_pass:message("Can't locate path to MSYS. This is needed for flex.")
} else:!exists($$MSYSPATH\\1.0\\bin\\flex.exe) {
    !build_pass:message("MSYSPATH is set but flex cannot be found.")
} else {
    FLEX_FILES = ppexpr.l
    flex.name = flex
    flex.input = FLEX_FILES
    flex.output = ${QMAKE_FILE_BASE}-lex.inc
    flex.commands = $$MSYSPATH\\1.0\\bin\\flex.exe --noline ${QMAKE_FILE_IN}
    flex.CONFIG += no_link explicit_dependencies
    QMAKE_EXTRA_COMPILERS += flex

    QLALR_FILES = ppexpr.g
    qlalr.name = qlalr
    qlalr.input = QLALR_FILES
    qlalr.output = ${QMAKE_FILE_BASE}_grammar.cpp
    qlalr.commands = $$[QT_INSTALL_BINS]\\qlalr.exe --no-lines ${QMAKE_FILE_IN}
    qlalr.depends = ${QMAKE_FILE_BASE}.l
    qlalr.dependency_type = TYPE_C
    qlalr.CONFIG += no_link explicit_dependencies
    QMAKE_EXTRA_COMPILERS += qlalr
}

HEADERS += fileinfo.h \
           helperfunctions.h \
           makefile.h \
           makefilefactory.h \
           makefilelinereader.h \
           macrotable.h \
           exception.h \
           dependencygraph.h \
           options.h \
           parser.h \
           preprocessor.h \
           ppexprparser.h \
           targetexecutor.h \
           cmdlexer.h \
           commandexecutor.h

SOURCES += fileinfo.cpp \
           helperfunctions.cpp \
           macrotable.cpp \
           makefile.cpp \
           makefilefactory.cpp \
           makefilelinereader.cpp \
           exception.cpp \
           dependencygraph.cpp \
           options.cpp \
           parser.cpp \
           preprocessor.cpp \
           ppexpr_grammar.cpp \
           ppexprparser.cpp \
           targetexecutor.cpp \
           cmdlexer.cpp \
           commandexecutor.cpp

