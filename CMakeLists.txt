cmake_minimum_required(VERSION 3.10)
project(ReliefCampManager CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

find_package(Threads REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
if(EXISTS ${CMAKE_SOURCE_DIR}/src/seed_main.cpp)
  list(REMOVE_ITEM LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/seed_main.cpp)
endif()

add_executable(relief ${LIB_SOURCES} ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(relief PRIVATE Threads::Threads)

file(GLOB TEST_SOURCES ${CMAKE_SOURCE_DIR}/tests/*.cpp)
add_executable(relief_tests ${LIB_SOURCES} ${TEST_SOURCES})
target_include_directories(relief_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(relief_tests PRIVATE Threads::Threads)

if(EXISTS ${CMAKE_SOURCE_DIR}/src/seed_main.cpp)
  add_executable(relief_seed ${LIB_SOURCES} ${CMAKE_SOURCE_DIR}/src/seed_main.cpp)
  target_link_libraries(relief_seed PRIVATE Threads::Threads)
endif()

enable_testing()
add_test(NAME all_tests COMMAND relief_tests)
