add_executable(connection
    main.c
)

target_compile_definitions(connection PRIVATE
    CLIENT_FILE="$<TARGET_FILE:client>"
    SERVER_FILE="${SERVER_FILE}"
)

endtest(connection)