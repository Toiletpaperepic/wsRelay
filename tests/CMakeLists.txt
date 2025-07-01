macro(endtest test)    
    message(STATUS "adding test ${test}")
    set_property(TARGET ${test} PROPERTY OUTPUT_NAME wsp-${test})

    add_test(NAME ${test} COMMAND ${test})

    set_property(TEST ${test}
                PROPERTY PASS_REGULAR_EXPRESSION "passed" "ok")
    set_property(TEST ${test}
                PROPERTY FAIL_REGULAR_EXPRESSION "Error" "Fail")
endmacro()


add_subdirectory(Connection)