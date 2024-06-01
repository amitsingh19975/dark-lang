cmake_minimum_required(VERSION 3.15)

project(dark CXX C)

include(cmake/StandardProjectSettings.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(project_options INTERFACE)
add_library(project_warnings INTERFACE)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

include(cmake/StaticAnalyzers.cmake)

include(cmake/LLVM.cmake)
include(cmake/SimdUTF.cmake)
include(cmake/GMP.cmake)

option(ENABLE_PCH "Enable Precompiled Headers" OFF)
if(ENABLE_PCH)
  target_precompile_headers(project_options INTERFACE <vector> <string> <map> <utility>)
endif(ENABLE_PCH)

add_library(dark_core INTERFACE)
target_include_directories(dark_core INTERFACE include)
target_link_libraries(dark_core INTERFACE simdutf::simdutf)

option(ENABLE_TESTING "Enable Test Builds" ON)

if(ENABLE_TESTING)
  enable_testing()
  message("Building Tests. Be sure to check out test/ directory")
  add_subdirectory(test)
endif(ENABLE_TESTING)

add_subdirectory(src)