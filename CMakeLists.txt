cmake_minimum_required(VERSION 4.0)
project(wsp VERSION 0.0.0 LANGUAGES C CXX)

add_subdirectory(utils)
add_subdirectory(websocket)
add_subdirectory(client)
add_subdirectory(server)

include(CTest)
add_subdirectory(tests)