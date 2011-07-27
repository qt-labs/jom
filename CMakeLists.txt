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

# some general definitions & the directories that should be included
add_definitions(${QT_DEFINITIONS})
include_directories(
  ${QT_INCLUDES}
  ${CMAKE_BINARY_DIR}
  src/jomlib
)
if(MSVC)
    add_definitions(
      -D_CRT_SECURE_NO_DEPRECATE
    )
   # Qt disables the native wchar_t type, do it too to avoid linking issues
   set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:wchar_t-" )
endif(MSVC)
add_definitions(
  -DQT_NO_CAST_FROM_ASCII
  -DQT_NO_CAST_TO_ASCII
  -DQT_USE_FAST_CONCATENATION
  -DQT_USE_FAST_OPERATOR_PLUS
)

# in subdirectory src:
# 
set(JOM_MOCS src/jomlib/targetexecutor.h
             src/jomlib/commandexecutor.h
             )

set(JOM_SRCS src/jomlib/commandexecutor.cpp
             src/jomlib/dependencygraph.cpp
             src/jomlib/exception.cpp
             src/jomlib/fileinfo.cpp
             src/jomlib/helperfunctions.cpp
             src/jomlib/macrotable.cpp
             src/jomlib/makefile.cpp
             src/jomlib/makefilefactory.cpp
             src/jomlib/makefilelinereader.cpp
             src/jomlib/options.cpp
             src/jomlib/parser.cpp
             src/jomlib/ppexpr_grammar.cpp
             src/jomlib/ppexprparser.cpp
             src/jomlib/preprocessor.cpp
             src/jomlib/targetexecutor.cpp
             )

# run moc on all headers and add the moc files to the SRCS
qt4_wrap_cpp(JOM_SRCS ${JOM_MOCS})
add_executable(jom src/app/main.cpp ${JOM_SRCS})
target_link_libraries(jom ${QT_QTMAIN_LIBRARY} ${QT_QTCORE_LIBRARY} ws2_32)

# install binaries to bin/, libraries to lib/ and import libraries to lib/ too
install(TARGETS jom  RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)

# if we want to use tests:
if(JOM_ENABLE_TESTS)
    # in subdirectory tests:
    qt4_wrap_cpp(JOM_SRCS tests/tests.h)
    set(TESTS_SRCS tests/tests.cpp)
    add_executable(jom-test tests.moc ${TESTS_SRCS} ${JOM_SRCS})
    target_link_libraries(jom-test ${QT_QTMAIN_LIBRARY} 
                                   ${QT_QTCORE_LIBRARY} 
                                   ${QT_QTTEST_LIBRARY})

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

