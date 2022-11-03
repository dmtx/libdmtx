cmake_minimum_required(VERSION 3.0)
project(DMTX VERSION 0.7.7 LANGUAGES C)

# DMTX library
option(DMTX_SHARED "Build DMTX as shared library" ${BUILD_SHARED_LIBS})
if(DMTX_SHARED)
  add_library(dmtx SHARED "dmtx.c")
else()
  add_library(dmtx STATIC "dmtx.c")
endif()

# Compiler specific settings
if (MSVC)
  set_target_properties(dmtx PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
  set_target_properties(dmtx PROPERTIES
                                VERSION ${PROJECT_VERSION}
                                SOVERSION ${PROJECT_VERSION_MAJOR})
  target_link_libraries(dmtx PUBLIC -lm)
endif()

set_target_properties(dmtx PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/dmtx.h")
install(TARGETS dmtx LIBRARY ARCHIVE RUNTIME PUBLIC_HEADER)

# Add tests if DMTX is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    if(BUILD_TESTING)
      add_subdirectory("test")
    endif()
endif()
