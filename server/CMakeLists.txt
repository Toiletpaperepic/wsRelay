find_program(PROGRAM_CARGO cargo REQUIRED)

include(ExternalProject)

ExternalProject_Add(server
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${PROGRAM_CARGO} build --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml --target-dir ${CMAKE_CURRENT_BINARY_DIR} $<IF:$<CONFIG:Release>,--release,--bins>
    INSTALL_COMMAND ""
    USES_TERMINAL_BUILD ON
    BUILD_ALWAYS ON
)