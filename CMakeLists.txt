cmake_minimum_required(VERSION 3.5)
project(DMTX VERSION 0.7.8 LANGUAGES C)

set(DEFAULT_SHARED_LIBS "ON")
if(DEFINED BUILD_SHARED_LIBS AND NOT BUILD_SHARED_LIBS STREQUAL "")
    set(DEFAULT_SHARED_LIBS "${BUILD_SHARED_LIBS}")
endif()
option(DMTX_SHARED "Build DMTX as shared library" "${DEFAULT_SHARED_LIBS}")
if(DMTX_SHARED)
  add_library(dmtx_shared SHARED "dmtx.c")
  if (MSVC)
    set_target_properties(dmtx_shared PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  else()
    set_target_properties(dmtx_shared PROPERTIES
      VERSION ${PROJECT_VERSION}
      SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    target_link_libraries(dmtx_shared PUBLIC m)
  endif()
  set_target_properties(dmtx_shared PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/dmtx.h" OUTPUT_NAME "dmtx")
  install(TARGETS dmtx_shared LIBRARY ARCHIVE RUNTIME PUBLIC_HEADER)
endif()

set(DEFAULT_STATIC_LIBS "ON") # XXX: ON for now, will default to OFF someday later (maybe on 0.7.9/0.8)
if(DEFINED BUILD_STATIC_LIBS AND NOT BUILD_STATIC_LIBS STREQUAL "")
  set(DEFAULT_STATIC_LIBS "${BUILD_STATIC_LIBS}")
endif()
option(DMTX_STATIC "Build DMTX as static library" "${DEFAULT_STATIC_LIBS}")
if(DMTX_STATIC)
  add_library(dmtx_static STATIC "dmtx.c")
  if (MSVC)
    set_target_properties(dmtx_static PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  else()
    target_link_libraries(dmtx_static PUBLIC m)
  endif()
  set_target_properties(dmtx_static PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/dmtx.h" OUTPUT_NAME "dmtx")
  install(TARGETS dmtx_static LIBRARY ARCHIVE RUNTIME PUBLIC_HEADER)
endif()

option(DMTX_READER_PROGRAMMING "Enable reader programming feature (adds breaking changes to API)" OFF)
if(DMTX_READER_PROGRAMMING)
  add_compile_definitions(HAVE_READER_PROGRAMMING)
endif()

add_compile_definitions(VERSION="${PROJECT_VERSION}")

# Add support for tests if DMTX is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "Build tests" OFF)
    if(BUILD_TESTING)
      add_subdirectory("test")
    endif()
    include(CTest)
endif()

# vim: ft=cmake ts=2 sw=2 sts=2 et
