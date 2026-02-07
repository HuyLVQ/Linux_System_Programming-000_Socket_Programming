cmake_minimum_required(VERSION 2.8.9)

project(fifo_ipc C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Make headers visible
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(server
    src/server.c
)

add_executable(client
    src/client.c
)

target_link_libraries(server rt pthread)
target_link_libraries(client rt pthread)
