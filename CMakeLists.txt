cmake_minimum_required(VERSION 3.31.6)
project(wsr VERSION 0.0.0 LANGUAGES C CXX)

add_subdirectory(utils)
add_subdirectory(websocket)
add_subdirectory(client)
add_subdirectory(server)

include(CTest)
add_subdirectory(tests)