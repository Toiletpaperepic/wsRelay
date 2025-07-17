option(BUILD_SERVER "build the server program." ON)
message(STATUS "Building with server: ${BUILD_SERVER}")

if(BUILD_SERVER)
    find_program(PROGRAM_CARGO cargo REQUIRED)

    include(ExternalProject)

    ExternalProject_Add(server
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${PROGRAM_CARGO} build --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml --target-dir ${CMAKE_CURRENT_BINARY_DIR} $<IF:$<CONFIG:Release>,--release,--bins>
        BUILD_COMMAND &&
        BUILD_COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<CONFIG:Release>,release,debug>/wsp-server ${CMAKE_CURRENT_BINARY_DIR}/wsp-server
        INSTALL_COMMAND ""
        BUILD_ALWAYS ON
        USES_TERMINAL_BUILD ON
    )

    set(SERVER_FILE ${CMAKE_CURRENT_BINARY_DIR}/wsp-server PARENT_SCOPE)
endif()