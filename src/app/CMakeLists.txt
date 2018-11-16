add_executable(jom
  application.cpp
  application.h
  main.cpp
  )

set_target_properties(jom PROPERTIES DEBUG_POSTFIX d)
target_link_libraries(jom PRIVATE jomlib)

install(TARGETS jom RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
