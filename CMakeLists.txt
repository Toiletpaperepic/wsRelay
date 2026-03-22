cmake_minimum_required(VERSION 3.31.6)
project(wsrelay VERSION 0.0.2 LANGUAGES C)
add_compile_definitions(__PROJECT_VERSION__="${CMAKE_PROJECT_VERSION}")

include(CheckSourceCompiles)
include(CheckIncludeFiles)
include(CheckSymbolExists)

set(INCLUDE_CHECK_LIST
    winsock2.h
    sys/epoll.h
    poll.h
    pthread.h
    stdckdint.h 
)

foreach(INCLUDE IN LISTS INCLUDE_CHECK_LIST)
    string(MAKE_C_IDENTIFIER ${INCLUDE} INCLUDE_C_IDENTIFIER)
    string(TOUPPER ${INCLUDE_C_IDENTIFIER} INCLUDE_C_IDENTIFIER)
    check_include_files(${INCLUDE} HAVE_${INCLUDE_C_IDENTIFIER})
    if(HAVE_${INCLUDE_C_IDENTIFIER})
        add_compile_definitions(HAVE_${INCLUDE_C_IDENTIFIER})
    endif()
endforeach()

set(SYMBOL_CHECK_LIST
    CreateThread
)

set(SYMBOL_HEADER_CHECK_LIST
    windows.h
)

foreach(SYMBOL INCLUDE IN ZIP_LISTS SYMBOL_CHECK_LIST SYMBOL_HEADER_CHECK_LIST)
    string(MAKE_C_IDENTIFIER ${SYMBOL} INCLUDE_C_IDENTIFIER)
    string(TOUPPER ${INCLUDE_C_IDENTIFIER} INCLUDE_C_IDENTIFIER)
    check_symbol_exists(${SYMBOL} ${INCLUDE} HAVE_${INCLUDE_C_IDENTIFIER})
    if(HAVE_${INCLUDE_C_IDENTIFIER})
        add_compile_definitions(HAVE_${INCLUDE_C_IDENTIFIER})
    endif()
endforeach()

check_source_compiles(C [[
    #include <stdint.h>
    int main() {
        int32_t a = INT32_MAX;
        int32_t b = 1;
        int32_t result;
        __builtin_add_overflow(a, b, &result); 
        return 0; 
    }
    ]]
    HAVE___BUILTIN_ADD_OVERFLOW
)

check_source_compiles(C [[
    #include <stdint.h>
    int main() {
        int32_t a = INT32_MAX;
        int32_t b = 2;
        int32_t result;
        __builtin_mul_overflow(a, b, &result); 
        return 0; 
    }
    ]]
    HAVE___BUILTIN_MUL_OVERFLOW
)

if(HAVE___BUILTIN_ADD_OVERFLOW)
    add_compile_definitions(HAVE___BUILTIN_ADD_OVERFLOW)
endif()

if(HAVE___BUILTIN_ADD_OVERFLOW)
    add_compile_definitions(HAVE___BUILTIN_MUL_OVERFLOW)
endif()

# ----------utils----------
add_library(base64
    utils/base64/base64.c
    utils/base64/base64.h
)

target_include_directories(base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils/base64)

include_directories(utils/common/include)
# -------------------------

# --------websocket--------
add_library(websocket SHARED
    websocket/dll_export.h
    websocket/http.c
    websocket/http.h
    websocket/opcodes.h
    websocket/parse_url.c
    websocket/parse_url.h
    websocket/status_codes.h
    websocket/websocket.c
    websocket/websocket.h
    websocket/wsrelay.h
)

target_include_directories(websocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/websocket)
target_link_libraries(websocket PUBLIC base64)
if(WIN32)
    find_library(WSOCK32_LIBRARY wsock32)
    find_library(WS2_32_LIBRARY ws2_32)
    find_library(BCRYPT_LIBRARY bcrypt)

    target_link_libraries(websocket PUBLIC ${WSOCK32_LIBRARY} ${WS2_32_LIBRARY} ${BCRYPT_LIBRARY})
endif()
set_target_properties(websocket PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}-websocket
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION 1
)
install(TARGETS websocket LIBRARY DESTINATION lib)
# -------------------------

# ---------client----------
add_executable(client
    client/main.c
    client/args.c
    client/args.h
    client/socket.c
    client/socket.h
)

target_link_libraries(client PUBLIC websocket)
set_property(TARGET client PROPERTY OUTPUT_NAME ${PROJECT_NAME})
install(TARGETS client RUNTIME DESTINATION bin)
# -------------------------

# ---------tests-----------
include(CTest)

macro(endtest testname)
    set_property(TARGET ${testname} PROPERTY OUTPUT_NAME ${PROJECT_NAME}-${testname}-test)
    target_include_directories(${testname} PUBLIC tests/include)
    add_test(NAME ${testname} COMMAND ${testname})
endmacro()

add_executable(commandline
    tests/commandline/main.c
    client/args.c
    client/args.h
)

endtest(commandline)
# -------------------------

include(GNUInstallDirs)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/utils/base64/LICENSE" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}/wpa_supplicant and hostapd")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
include(CPack)