cmake_minimum_required(VERSION 3.12)

enable_testing()

link_directories(../build)

if(WIN32)
  # The folder where the files (so/a/dll) of the dmtx library are located
  set(DMTX_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../build/*dmtx.*") 
  
  file(COPY ${CMAKE_CURRENT_LIST_DIR}/../dmtx.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(GLOB LIBS ${DMTX_LIB_DIR})
  file(COPY ${LIBS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif(WIN32)

add_executable(test_simple
  "simple_test/simple_test.c")
target_link_libraries(test_simple PRIVATE dmtx m)
add_test(NAME test_simple COMMAND $<TARGET_FILE:test_simple>)


add_executable(test_unit
  "unit_test/unit_test.c")
target_link_libraries(test_unit PRIVATE dmtx m)
add_test(NAME test_unit COMMAND $<TARGET_FILE:test_unit>)