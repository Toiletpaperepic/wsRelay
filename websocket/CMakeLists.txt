add_library(websocket SHARED
    websocket.c
    websocket.h
    parse_url.c
    parse_url.h
)

target_include_directories(websocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(websocket PUBLIC base64 m)
set_property(TARGET websocket PROPERTY OUTPUT_NAME wsp-websocket)