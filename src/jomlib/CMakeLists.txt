add_library(jomlib STATIC
  commandexecutor.cpp
  commandexecutor.h
  dependencygraph.cpp
  dependencygraph.h
  exception.cpp
  exception.h
  fastfileinfo.cpp
  fastfileinfo.h
  filetime.cpp
  filetime.h
  helperfunctions.cpp
  helperfunctions.h
  iocompletionport.cpp
  iocompletionport.h
  jobclient.cpp
  jobclient.h
  jobclientacquirehelper.cpp
  jobclientacquirehelper.h
  jobserver.cpp
  jomprocess.cpp
  jomprocess.h
  macrotable.cpp
  macrotable.h
  makefile.cpp
  makefile.h
  makefilefactory.cpp
  makefilefactory.h
  makefilelinereader.cpp
  makefilelinereader.h
  options.cpp
  options.h
  parser.cpp
  parser.h
  ppexpr_grammar.cpp
  ppexpr_grammar_p.h
  ppexprparser.cpp
  ppexprparser.h
  preprocessor.cpp
  preprocessor.h
  stable.h
  targetexecutor.cpp
  targetexecutor.h
  )

target_include_directories(jomlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(jomlib PUBLIC Qt5::Core)
target_compile_definitions(jomlib PRIVATE
  QT_NO_CAST_FROM_ASCII
  QT_NO_CAST_TO_ASCII
  UNICODE
  )

if(MSVC)
  target_compile_definitions(jomlib PUBLIC
    _CRT_SECURE_NO_WARNINGS
    )
endif()
