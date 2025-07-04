macro(endtest test)
    set_property(TARGET ${test} PROPERTY OUTPUT_NAME wsp-${test}-test)

    add_test(NAME ${test} COMMAND ${test})
endmacro()


add_subdirectory(Connection)