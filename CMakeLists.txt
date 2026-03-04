cmake_minimum_required(VERSION 3.31.6)
project(wsrelay VERSION 0.0.1 LANGUAGES C CXX)
add_compile_definitions(__PROJECT_VERSION__="${CMAKE_PROJECT_VERSION}")

# ----------utils----------
add_library(base64
    utils/base64/base64.c
    utils/base64/base64.h
)

target_include_directories(base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils/base64)

add_library(commoninclude INTERFACE)
target_include_directories(commoninclude INTERFACE utils/common/include)
# -------------------------

# --------websocket--------
add_library(websocket SHARED
    websocket/opcodes.h
    websocket/websocket.c
    websocket/websocket.h
    websocket/parse_url.c
    websocket/parse_url.h
    websocket/http.c
    websocket/http.h
)

target_include_directories(websocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/websocket)
target_link_libraries(websocket PUBLIC base64 commoninclude)
set_property(TARGET websocket PROPERTY OUTPUT_NAME ${PROJECT_NAME}-websocket)
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