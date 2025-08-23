cmake_minimum_required(VERSION 3.12)

enable_testing()

link_directories(../build)

set(DMTX_BUILD_LIB ${CMAKE_CURRENT_LIST_DIR}/../build/libdmtx${CMAKE_SHARED_LIBRARY_SUFFIX})
set(DMTX_RAW_H ${CMAKE_CURRENT_LIST_DIR}/../dmtx.h)

set(DMTX_H ${CMAKE_CURRENT_LIST_DIR}/include/dmtx)
set(DMTX_LIB ${CMAKE_CURRENT_LIST_DIR}/build/libdmtx${CMAKE_SHARED_LIBRARY_SUFFIX})

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

# Copy share lib and header files
file(COPY ${DMTX_BUILD_LIB} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/build)
file(COPY ${DMTX_RAW_H} DESTINATION ${DMTX_H})

add_executable(test_simple
  "simple_test/simple_test.c")

target_include_directories(test_simple PUBLIC INTERFACE BEFORE ${DMTX_H})
target_link_libraries(test_simple PRIVATE ${DMTX_LIB} m)
add_test(NAME test_simple COMMAND $<TARGET_FILE:test_simple>)

add_executable(test_unit
  "unit_test/unit_test.c")
target_include_directories(test_unit PUBLIC INTERFACE BEFORE ${DMTX_H})
target_link_libraries(test_unit PRIVATE ${DMTX_LIB} m)
add_test(NAME test_unit COMMAND $<TARGET_FILE:test_unit>)
