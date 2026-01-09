if(WIN32)
    include(FetchContent)

    FetchContent_Declare(
        wepoll
        GIT_REPOSITORY https://github.com/piscisaureus/wepoll
        GIT_TAG        v1.5.8
    )

    FetchContent_MakeAvailable(wepoll)
    
    add_library(wepoll
        ${wepoll_SOURCE_DIR}/wepoll.c
        ${wepoll_SOURCE_DIR}/wepoll.h
    )
    target_include_directories(wepoll PUBLIC ${wepoll_SOURCE_DIR})
endif()

add_library(base64
    base64/base64.c
    base64/base64.h
)

target_include_directories(base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/base64)
target_compile_options(base64 PRIVATE -fPIC)
install(TARGETS base64 LIBRARY DESTINATION lib)