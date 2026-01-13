add_executable(client
    main.c
    main.h
    args.c
    args.h
    socket.c
    socket.h
)

target_link_libraries(client PUBLIC websocket)
set_property(TARGET client PROPERTY OUTPUT_NAME wsr-client)
set_property(TARGET client PROPERTY C_STANDARD 23)
install(TARGETS client RUNTIME DESTINATION bin)