find_program(PROGRAM_CARGO cargo REQUIRED)

add_custom_target(server 
    COMMAND ${PROGRAM_CARGO} build --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml --target-dir ${CMAKE_CURRENT_BINARY_DIR}
)