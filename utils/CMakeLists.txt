add_library(base64 SHARED
    base64/base64-cstring.cpp
    base64/base64-cstring.h
    base64/cpp-base64/base64.cpp
    base64/cpp-base64/base64.h
)

target_include_directories(base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/base64)