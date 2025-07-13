cmake_minimum_required(VERSION 4.0)
project(wsp)

include_directories(include)

add_subdirectory(websocket)
add_subdirectory(client)
add_subdirectory(server)

include(CTest)
add_subdirectory(tests)