macro(endtest test)
    set_property(TARGET ${test} PROPERTY OUTPUT_NAME ${PROJECT_NAME}-${test}-test)
    target_include_directories(${test} PUBLIC ../include)

    add_test(NAME ${test} COMMAND ${test})
endmacro()

add_subdirectory(commandline)