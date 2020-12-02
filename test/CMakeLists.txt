function(expected_add_test name)
    add_executable(${name} "${name}.cpp")
    target_link_libraries(${name} PRIVATE expected Boost::unit_test_framework Boost::dynamic_linking)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

expected_add_test(core_test)
