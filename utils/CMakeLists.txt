add_library(base64
    base64/base64.c
    base64/base64.h
)

target_include_directories(base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/base64)
install(TARGETS base64 LIBRARY DESTINATION lib)