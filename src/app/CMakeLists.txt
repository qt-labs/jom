file(READ version.txt version)
string(REGEX REPLACE "[\r\n]" "" version ${version})
string(REPLACE "." ";" version_list ${version})
list(GET version_list 0 JOM_VERSION_MAJOR)
list(GET version_list 1 JOM_VERSION_MINOR)
list(GET version_list 2 JOM_VERSION_PATCH)

configure_file(
    app.rc.in
    app.rc)

add_executable(jom
  application.cpp
  application.h
  main.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/app.rc
  )

target_compile_definitions(jom
    PRIVATE JOM_VERSION_MAJOR=${JOM_VERSION_MAJOR}
    PRIVATE JOM_VERSION_MINOR=${JOM_VERSION_MINOR}
    PRIVATE JOM_VERSION_PATCH=${JOM_VERSION_PATCH}
    )

set_target_properties(jom PROPERTIES DEBUG_POSTFIX d)
target_link_libraries(jom PRIVATE jomlib)

install(TARGETS jom RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
