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

# If we're building against a static Qt on Windows,
# we must link manually against all private libraries.
# This should not be necessary. See QTBUG-38913.
get_target_property(qt_core_type Qt5::Core TYPE)
if(qt_core_type MATCHES STATIC_LIBRARY)
    target_link_libraries(jomlib PRIVATE mincore userenv winmm ws2_32)

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(debug_suffix d)
        set(qt_build_config DEBUG)
    else()
        set(debug_suffix "")
        set(qt_build_config RELEASE)
    endif()

    get_target_property(qt_core_lib_location Qt5::Core IMPORTED_LOCATION_${qt_build_config})
    get_filename_component(qt_core_lib_dir ${qt_core_lib_location} DIRECTORY)
    target_link_libraries(jomlib PRIVATE "${qt_core_lib_dir}/qtpcre2${debug_suffix}.lib")
endif()

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
