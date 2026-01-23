add_library(base64
    base64/base64.c
    base64/base64.h
)

target_include_directories(base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/base64)
add_library(commoninclude INTERFACE)
target_include_directories(commoninclude INTERFACE common/include)