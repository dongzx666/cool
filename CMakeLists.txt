cmake_minimum_required(VERSION 2.8...3.13)
project(cool)

include(config/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations"
)

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
    src/iomanager.cpp
    src/timer.cpp
    src/hook.cpp
    src/fd_manager.cpp
    src/address.cpp
    src/socket.cpp
    src/stream.cpp
    src/socket_stream.cpp
    src/tcp_server.cpp
    src/bytearray.cpp
    src/http/http.cpp
    src/http/http_parser.cpp
    src/http/http_session.cpp
    src/http/http_server.cpp
    src/http/servlet.cpp)

ragelmaker(src/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
# ragelmaker(src/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(src SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(src)

set(LIBS src yaml-cpp pthread dl)

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

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager src)
target_link_libraries(test_iomanager ${LIBS})
force_redefine_file_macro_for_sources(test_iomanager)

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook src)
target_link_libraries(test_hook ${LIBS})
force_redefine_file_macro_for_sources(test_hook)

add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address src)
target_link_libraries(test_address ${LIBS})
force_redefine_file_macro_for_sources(test_address)

add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket src)
target_link_libraries(test_socket ${LIBS})
force_redefine_file_macro_for_sources(test_socket)

add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray src)
target_link_libraries(test_bytearray ${LIBS})
force_redefine_file_macro_for_sources(test_bytearray)

add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http src)
target_link_libraries(test_http ${LIBS})
force_redefine_file_macro_for_sources(test_http)

add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser src)
target_link_libraries(test_http_parser ${LIBS})
force_redefine_file_macro_for_sources(test_http_parser)

add_executable(test_tcp_server tests/test_tcp_server.cpp)
add_dependencies(test_tcp_server src)
target_link_libraries(test_tcp_server ${LIBS})
force_redefine_file_macro_for_sources(test_tcp_server)

add_executable(echo_server samples/echo_server.cpp)
add_dependencies(echo_server src)
target_link_libraries(echo_server ${LIBS})
force_redefine_file_macro_for_sources(echo_server)

add_executable(test_http_server tests/test_http_server.cpp)
add_dependencies(test_http_server src)
target_link_libraries(test_http_server ${LIBS})
force_redefine_file_macro_for_sources(test_http_server)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
