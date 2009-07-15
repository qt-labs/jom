project(jom)
cmake_minimum_required(VERSION 2.6)


option(JOM_ENABLE_TESTS "Enable unit-testing for jom" OFF)

if(JOM_ENABLE_TESTS)
    enable_testing()
endif(JOM_ENABLE_TESTS)


# where to look first for cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules;${CMAKE_MODULE_PATH}")

# search for Qt4 - it is required
find_package(Qt4 REQUIRED)

# search for libantlr - call our own FindANTLR.cmake-module in cmake/modules
# this will define a variable called ANTLR_FOUND which we can use to determine
# whether ANTLR should be compiled in
find_package(ANTLR)

# some general definitions & the directories that should be included
add_definitions(${QT_DEFINITIONS})
include_directories(${QT_INCLUDES} src/app ${CMAKE_BINARY_DIR})

# when building in Debug mode, we shouldn't link in MSVCRT by default
if(MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
    endif(CMAKE_BUILD_TYPE STREQUAL Debug)
endif(MSVC)


# in subdirectory src:
# 
set(JOM_MOCS src/app/targetexecutor.h
             src/app/commandexecutor.h)

set(JOM_SRCS src/app/macrotable.cpp
             src/app/makefile.cpp
             src/app/exception.cpp
             src/app/dependencygraph.cpp
             src/app/options.cpp
             src/app/parser.cpp
             src/app/preprocessor.cpp
             src/app/targetexecutor.cpp
             src/app/commandexecutor.cpp
             src/app/makefilelinereader.cpp)

if(ANTLR_FOUND)
    # if antlr library was found, simply add the definition, include the include directory and add some more files
    add_definitions(-DANTLR_AVAILABLE)

    include_directories(${ANTLR_INCLUDE_DIR})

    set(JOM_SRCS ${JOM_SRCS}
                 src/app/ppexpr/ppexpression.cpp
                 src/app/ppexpr/NMakeExpressionLexer.c
                 src/app/ppexpr/NMakeExpressionParser.c)
endif(ANTLR_FOUND)

# run moc on all headers and add the moc files to the SRCS
qt4_wrap_cpp(JOM_SRCS ${JOM_MOCS})
add_executable(jom src/app/main.cpp ${JOM_SRCS})
target_link_libraries(jom ${QT_QTMAIN_LIBRARY} ${QT_QTCORE_LIBRARY} ws2_32)
if(ANTLR_FOUND)
    target_link_libraries(jom ${ANTLR_LIBRARIES})
endif(ANTLR_FOUND)



set(MEXT_MOCS src/mext/consoleoutprocess.h)
set(MEXT_SRCS src/mext/main.cpp
              src/mext/consoleoutprocess.cpp)

qt4_wrap_cpp(MEXT_SRCS ${MEXT_MOCS})
add_executable(mext ${MEXT_SRCS})
target_link_libraries(mext ${QT_QTMAIN_LIBRARY} ${QT_QTCORE_LIBRARY} ws2_32)


# install binaries to bin/, libraries to lib/ and import libraries to lib/ too
install(TARGETS jom mext RUNTIME DESTINATION bin
                         LIBRARY DESTINATION lib
                         ARCHIVE DESTINATION lib)

# if we want to use tests:
if(JOM_ENABLE_TESTS)
    # in subdirectory tests:
    set(TESTS_SRCS tests/tests.cpp)
    qt4_generate_moc(tests/tests.cpp ${CMAKE_BINARY_DIR}/tests.moc)
    add_executable(jom-test tests.moc ${TESTS_SRCS} ${JOM_SRCS})
    target_link_libraries(jom-test ${QT_QTMAIN_LIBRARY} 
                                   ${QT_QTCORE_LIBRARY} 
                                   ${QT_QTTEST_LIBRARY})
    if(ANTLR_FOUND)
        target_link_libraries(jom-test ${ANTLR_LIBRARIES})
    endif(ANTLR_FOUND)

    # copy the data directory 'makefiles' over into the build directory as the tests should be run from there
    file(GLOB_RECURSE JOM_TEST_DATA RELATIVE ${CMAKE_SOURCE_DIR}/tests/makefiles/ "tests/makefiles/*")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/makefiles)
    foreach(_file ${JOM_TEST_DATA})
        configure_file(${CMAKE_SOURCE_DIR}/tests/makefiles/${_file} ${CMAKE_BINARY_DIR}/makefiles/${_file} COPY_ONLY)
    endforeach(_file ${JOM_TEST_DATA})

    # add one unit test per function:
    # you can run the unittests all at once using 'make test' from the build directory
    add_test(includeFiles jom-test includeFiles)
    add_test(includeCycle jom-test includeCycle)
    add_test(macros jom-test macros)
    add_test(inferenceRules jom-test inferenceRules)
    add_test(cycleInTargets jom-test cycleInTargets)
    add_test(multipleTargets jom-test multipleTargets)
    add_test(multipleTargetsFail jom-test multipleTargetsFail)
    add_test(comments jom-test comments)
    add_test(fileNameMacros jom-test fileNameMacros)

endif(JOM_ENABLE_TESTS)

