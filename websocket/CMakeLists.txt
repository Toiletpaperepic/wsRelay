add_library(websocket SHARED
    websocket.c
    websocket.h
)

target_include_directories(websocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET websocket PROPERTY OUTPUT_NAME wsp-websocket)