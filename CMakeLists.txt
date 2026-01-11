cmake_minimum_required(VERSION 3.31.6)
project(wsr VERSION 0.0.0 LANGUAGES C CXX)

add_compile_options("$<IF:$<CONFIG:Debug>,-D__Debug__,-D__Release__>")

add_subdirectory(utils)
add_subdirectory(websocket)
add_subdirectory(client)

include(CTest)
add_subdirectory(tests)