macro(endtest test)
    set_property(TARGET ${test} PROPERTY OUTPUT_NAME wsr-${test}-test)
    target_include_directories(${test} PUBLIC ../include)

    add_test(NAME ${test} COMMAND ${test})
endmacro()

# fixme: add_subdirectory(connection)

add_subdirectory(commandline)