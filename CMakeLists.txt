cmake_minimum_required(VERSION 3.6)
project(jom LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
enable_testing()

# where to look first for cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules;${CMAKE_MODULE_PATH}")

find_package(Qt5 5.2.0 REQUIRED COMPONENTS Core)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
add_subdirectory(src/jomlib)
add_subdirectory(src/app)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
