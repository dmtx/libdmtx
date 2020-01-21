cmake_minimum_required(VERSION 3.12)

enable_testing()

add_executable(test_simple
  "simple_test/simple_test.c")
target_link_libraries(test_simple PRIVATE dmtx)
add_test(NAME test_simple COMMAND $<TARGET_FILE:test_simple>)

add_executable(test_unit
  "unit_test/unit_test.c")
target_link_libraries(test_unit PRIVATE dmtx)
add_test(NAME test_unit COMMAND $<TARGET_FILE:test_unit>)
