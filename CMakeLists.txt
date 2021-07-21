cmake_minimum_required(VERSION 2.8...3.13)
project(cool)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

include_directories(.)
link_directories(/home/dongzx/opt/software/yaml-cpp/build)
include_directories(/home/dongzx/opt/software/yaml-cpp/include/yaml-cpp)

set(LIB_SRC
  src/log.cpp
  src/util.cpp
  src/config.cpp
  )

add_library(src SHARED ${LIB_SRC})

set(LIBS
  src
  # yaml-cpp
)
# add_executable(test_log "tests/test_log.cpp" src "${LIBS}")
add_executable(test_log tests/test_log.cpp)
add_dependencies(test_log src)
target_link_libraries(test_log src)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config src)
target_link_libraries(test_config src yaml-cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
