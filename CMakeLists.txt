cmake_minimum_required(VERSION 4.0)
project(wsp)

add_compile_definitions($<IF:$<CONFIG:Release>,__release__,__debug__>)
include_directories(include)

add_subdirectory(utils)
add_subdirectory(websocket)
add_subdirectory(client)
add_subdirectory(server)

include(CTest)
add_subdirectory(tests)