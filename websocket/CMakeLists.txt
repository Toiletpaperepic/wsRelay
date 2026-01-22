add_library(websocket SHARED
    opcodes.h
    websocket.c
    websocket.h
    parse_url.c
    parse_url.h
    http_header.c
    http_header.h
)

target_include_directories(websocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(websocket PUBLIC base64)
set_property(TARGET websocket PROPERTY OUTPUT_NAME wsr-websocket)
set_property(TARGET websocket PROPERTY C_STANDARD 23)
install(TARGETS websocket LIBRARY DESTINATION lib)