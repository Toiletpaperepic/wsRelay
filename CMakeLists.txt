cmake_minimum_required(VERSION 4.0)

project(wsp)

add_subdirectory(client)
add_subdirectory(server)

include(CTest)
add_subdirectory(tests)