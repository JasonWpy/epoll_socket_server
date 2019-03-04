cmake_minimum_required(VERSION 3.13)
project(epoll_socket_server)

set(CMAKE_CXX_STANDARD 14)

include_directories(/usr/local/include)

add_executable(epoll_socket_server
        README.md server.cpp server.h socket.cpp socket.h awe_thread.h awe_mutex.cpp awe_mutex.h)
