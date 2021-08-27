cmake_minimum_required(VERSION 2.8...3.13)
project(cool)

include (config/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

include_directories(.)
include_directories(/home/dongzx/opt/software/yaml-cpp/include/yaml-cpp)
link_directories(/home/dongzx/opt/software/yaml-cpp/build)


set(LIB_SRC
  src/log.cpp
  src/util.cpp
  src/config.cpp
  src/thread.cpp
  src/fiber.cpp
  src/scheduler.cpp
  )

add_library(src SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(src)

set(LIBS
  src
  yaml-cpp
  pthread
)
# add_executable(test_log "tests/test_log.cpp" src "${LIBS}")

add_executable(test_log tests/test_log.cpp)
add_dependencies(test_log src)
target_link_libraries(test_log ${LIBS})
force_redefine_file_macro_for_sources(test_log)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config src)
target_link_libraries(test_config ${LIBS})
force_redefine_file_macro_for_sources(test_config)

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread src)
target_link_libraries(test_thread ${LIBS})
force_redefine_file_macro_for_sources(test_thread)

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util src)
target_link_libraries(test_util ${LIBS})
force_redefine_file_macro_for_sources(test_util)

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber src)
target_link_libraries(test_fiber ${LIBS})
force_redefine_file_macro_for_sources(test_fiber)

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler src)
target_link_libraries(test_scheduler ${LIBS})
force_redefine_file_macro_for_sources(test_scheduler)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
