add_library(websocket SHARED
    websocket.c
    websocket.h
    parse_url.c
    parse_url.h
)

target_include_directories(websocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(websocket PUBLIC base64)
set_property(TARGET websocket PROPERTY OUTPUT_NAME wsp-websocket)

message(CHECK_START "Checking if libm is needed")
try_compile(LIBM_NEEDED SOURCE_FROM_VAR test.c "#include <math.h> void main() {log10(); return 0}")
if(NOT LIBM_NEEDED)
    message(CHECK_PASS "Yes")
    target_link_libraries(websocket PUBLIC m)
elseif()
    message(CHECK_FAIL "No")
endif()