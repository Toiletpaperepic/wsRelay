add_executable(client
    main.cpp
    socket.cpp
    websocket.cpp
)

find_package(websocketpp REQUIRED)
target_link_libraries(client PUBLIC websocketpp::websocketpp)

set_property(TARGET client PROPERTY OUTPUT_NAME wsp-client)