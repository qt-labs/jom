TEMPLATE = lib
TARGET = jomlib
DESTDIR = ../../lib
QT = core
CONFIG += qt staticlib debug_and_release

build_pass:CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
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
           ppexpr/ppexprparser.h \
           ppexpr/ppexpr_grammar_p.h \
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
           ppexpr/ppexpr_grammar.cpp \
           ppexpr/ppexprparser.cpp \
           targetexecutor.cpp \
           cmdlexer.cpp \
           commandexecutor.cpp

