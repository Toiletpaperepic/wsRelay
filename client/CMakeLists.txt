cmake_minimum_required(VERSION 4.0)
project(client)

find_package(websocketpp REQUIRED)

add_executable(${PROJECT_NAME} 
    ${PROJECT_SOURCE_DIR}/main.cpp
    ${PROJECT_SOURCE_DIR}/websocket.cpp
)