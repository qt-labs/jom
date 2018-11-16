find_package(Qt5 5.2.0 REQUIRED COMPONENTS Test)
add_executable(jom-test
  tests.cpp tests.h)

target_link_libraries(jom-test PRIVATE jomlib Qt5::Test)

add_test(NAME jom-test COMMAND jom-test)

add_custom_command(TARGET jom-test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/makefiles $<TARGET_FILE_DIR:jom-test>/makefiles
)
