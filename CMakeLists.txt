cmake_minimum_required(VERSION 3.31.6)
project(wsrelay VERSION 0.0.2 LANGUAGES C)
set(CMAKE_C_STANDARD 99)

add_compile_definitions(
    # SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P}
    __PROJECT_VERSION__="${CMAKE_PROJECT_VERSION}"
    OS_NAME="${CMAKE_HOST_SYSTEM_NAME}"
    PROCESSOR_ARCHITECTURE="${CMAKE_HOST_SYSTEM_PROCESSOR}"
    COMPILER_NAME="$<IF:$<STREQUAL:${CMAKE_C_COMPILER_ID},GNU>,gcc,${CMAKE_C_COMPILER_ID}>"
    COMPILER_VERSION="$<C_COMPILER_VERSION>"
)

include(CheckSourceCompiles)
include(CheckIncludeFiles)
include(CheckSymbolExists)

set(INCLUDE_CHECK_LIST
    sys/epoll.h
    poll.h
    pthread.h
    stdckdint.h 
)

foreach(INCLUDE IN LISTS INCLUDE_CHECK_LIST)
    # message(STATUS "${INCLUDE}")
    string(MAKE_C_IDENTIFIER ${INCLUDE} INCLUDE_C_IDENTIFIER)
    string(TOUPPER ${INCLUDE_C_IDENTIFIER} INCLUDE_C_IDENTIFIER)
    check_include_files(${INCLUDE} HAVE_${INCLUDE_C_IDENTIFIER})
    if(HAVE_${INCLUDE_C_IDENTIFIER})
        add_compile_definitions(HAVE_${INCLUDE_C_IDENTIFIER})
    endif()
endforeach()

set(SYMBOL_CHECK_LIST CreateThread WSAPoll)
set(SYMBOL_HEADER_CHECK_LIST windows.h winsock2.h)
set(SYMBOL_REQUIRED_LIBRARY_LIST NONE ws2_32)

foreach(SYMBOL INCLUDE LIBRARY IN ZIP_LISTS SYMBOL_CHECK_LIST SYMBOL_HEADER_CHECK_LIST SYMBOL_REQUIRED_LIBRARY_LIST)
    # message(STATUS "${SYMBOL} ${INCLUDE}")
    string(MAKE_C_IDENTIFIER ${SYMBOL} INCLUDE_C_IDENTIFIER)
    string(TOUPPER ${INCLUDE_C_IDENTIFIER} INCLUDE_C_IDENTIFIER)
    if(NOT "NONE" STREQUAL ${LIBRARY})
        set(CMAKE_REQUIRED_LIBRARIES ${LIBRARY})
        check_symbol_exists(${SYMBOL} ${INCLUDE} HAVE_${INCLUDE_C_IDENTIFIER})
        unset(CMAKE_REQUIRED_LIBRARIES)
    else()
        check_symbol_exists(${SYMBOL} ${INCLUDE} HAVE_${INCLUDE_C_IDENTIFIER})
    endif()
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

add_library(common SHARED
    common/strtouint.c
    common/strtouint.h
    common/logger.c
    common/logger.h
    common/macros.h
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)
set_target_properties(common PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}-common
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION 1
)
install(TARGETS common LIBRARY DESTINATION lib)

if(NOT HAVE___BUILTIN_ADD_OVERFLOW AND NOT HAVE___BUILTIN_MUL_OVERFLOW AND NOT HAVE_STDCKDINT_H)
    file(DOWNLOAD https://github.com/jart/jtckdint/raw/refs/heads/master/jtckdint.h ${CMAKE_CURRENT_BINARY_DIR}/downloads/include/jtckdint.h SHOW_PROGRESS EXPECTED_HASH SHA256=4a2bbbf4992384c17a332bfb3b4246f90bb84aa9b3282e1b966091981f1c2bb7)
    file(DOWNLOAD https://github.com/jart/jtckdint/raw/refs/heads/master/LICENSE.txt ${CMAKE_CURRENT_BINARY_DIR}/downloads/license/jtckdint/LICENSE SHOW_PROGRESS EXPECTED_HASH SHA256=c2b829605ee9c0b05c5c3708f308fc9b0954fb6ad1e0ab1add85f740a8827497)
    target_include_directories(common PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/downloads/include)
endif()
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
target_link_libraries(websocket PUBLIC base64 common)
if(WIN32)
    find_library(WS2_32_LIBRARY ws2_32)
    find_library(BCRYPT_LIBRARY bcrypt)

    target_link_libraries(websocket PUBLIC ${WS2_32_LIBRARY} ${BCRYPT_LIBRARY})
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

target_link_libraries(client PUBLIC websocket common)
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

target_link_libraries(commandline PUBLIC common)
endtest(commandline)

add_executable(parse_url
    tests/parse_url/parse_url.c
    websocket/parse_url.c
    websocket/parse_url.h
)

target_link_libraries(parse_url PUBLIC common)
endtest(parse_url)

# -------------------------

include(GNUInstallDirs)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/utils/base64/LICENSE" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}/wpa_supplicant and hostapd")
if(NOT HAVE___BUILTIN_ADD_OVERFLOW AND NOT HAVE___BUILTIN_MUL_OVERFLOW AND NOT HAVE_STDCKDINT_H)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/downloads/license/jtckdint/LICENSE" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}/jtckdint")
endif()
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
include(CPack)
