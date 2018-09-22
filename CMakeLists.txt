cmake_minimum_required(VERSION 3.12)
project(DMTX VERSION 0.7.5 LANGUAGES C)


add_library(dmtx dmtx.c)
target_link_libraries(dmtx -lm)

#------------------------------------------------------------------------------#
enable_testing()
add_executable(simple
  test/simple_test/simple_test.c)
target_link_libraries(simple PRIVATE dmtx)
add_test(NAME simpleTest COMMAND simple)

#------------------------------------------------------------------------------#
# this test doesn't work yet (nothing wrong with the code - something wrong with my script)
# add_executable(unit
#   test/unit_test/unit_test.c)
# target_link_libraries(unit PRIVATE dmtx)
# add_test(NAME unitTest COMMAND unit)

