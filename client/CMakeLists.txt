add_executable(client
    main.c
    args.c
    args.h
    socket.c
    socket.h
)

target_link_libraries(client PUBLIC websocket)
set_property(TARGET client PROPERTY OUTPUT_NAME ${PROJECT_NAME})
install(TARGETS client RUNTIME DESTINATION bin)