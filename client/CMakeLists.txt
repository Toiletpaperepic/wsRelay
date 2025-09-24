add_executable(client
    main.c
    main.h
    socket.c
    socket.h
)

target_link_libraries(client PUBLIC websocket)
set_property(TARGET client PROPERTY OUTPUT_NAME wsp-client)
set_property(TARGET client PROPERTY C_STANDARD 23)
install(TARGETS client RUNTIME DESTINATION bin)