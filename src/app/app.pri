HEADERS += makefile.h \
           macrotable.h \
           exception.h \
           dependencygraph.h \
           options.h \
           parser.h \
           preprocessor.h \
           targetexecutor.h \
           commandexecutor.h

SOURCES += macrotable.cpp \
           makefile.cpp \
           exception.cpp \
           dependencygraph.cpp \
           options.cpp \
           parser.cpp \
           preprocessor.cpp \
           targetexecutor.cpp \
           commandexecutor.cpp

# If you want to compile this project with support 
# for advanced preprocessor directives, you must compile 
# the C runtime for ANTLR 3. Then you must create a file antlr_config.pri
# which contains the definition of LIBANTLRDIR. Example:
# LIBANTLRDIR = C:/dev/libs/libantlr3c-3.1.1
exists(antlr_config.pri) {
    include(antlr_config.pri)
    exists($$LIBANTLRDIR/Release/antlr3c.lib) {
        DEFINES += ANTLR_AVAILABLE
        INCLUDEPATH += $$LIBANTLRDIR/include
    }
}

contains(DEFINES, ANTLR_AVAILABLE) {
    HEADERS += ppexpr/ppexpression.h \
               ppexpr/NMakeExpressionLexer.h \
               ppexpr/NMakeExpressionParser.h
    SOURCES += ppexpr/ppexpression.cpp \
               ppexpr/NMakeExpressionLexer.c \
               ppexpr/NMakeExpressionParser.c

    build_pass:CONFIG(debug, debug|release) {
        LIBS += $$LIBANTLRDIR/Debug/antlr3cd.lib
    }

    build_pass:CONFIG(release, debug|release) {
        LIBS += $$LIBANTLRDIR/Release/antlr3c.lib
    }
}

