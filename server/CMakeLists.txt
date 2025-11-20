option(BUILD_SERVER "build server program." ON)
message(STATUS "Build with server: ${BUILD_SERVER}")

if(BUILD_SERVER)
    find_program(PROGRAM_CARGO cargo REQUIRED)

    include(ExternalProject)

    ExternalProject_Add(server
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${PROGRAM_CARGO} build --manifest-path <SOURCE_DIR>/Cargo.toml --target-dir <BINARY_DIR> $<IF:$<CONFIG:Release>,--release,--bins>
        COMMAND ${CMAKE_COMMAND} -E copy <BINARY_DIR>/$<IF:$<CONFIG:Release>,release,debug>/wsr-server ${CMAKE_CURRENT_BINARY_DIR}/wsr-server
        INSTALL_COMMAND ""
        BUILD_ALWAYS ON
        USES_TERMINAL_BUILD ON
    )

    set(SERVER_FILE ${CMAKE_CURRENT_BINARY_DIR}/wsr-server)
    install(PROGRAMS ${SERVER_FILE} DESTINATION bin)
endif()