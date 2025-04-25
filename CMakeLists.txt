cmake_minimum_required(VERSION 3.20)

project(TavrosEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_policy(SET CMP0063 NEW) # POSITION_INDEPENDENT_CODE visibility
cmake_policy(SET CMP0069 NEW) # Enable LTO
cmake_policy(SET CMP0076 NEW)  # target_sources() and PRIVATE/PUBLIC/INTERFACE
cmake_policy(SET CMP0079 NEW)  # target_link_libraries() better work with ALIAS

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (TAV_COMPILE_OPTIONS)
    separate_arguments(TAV_COMPILE_OPTIONS)
    message(STATUS "Using extra compile options: ${TAV_COMPILE_OPTIONS}")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/tools/cmake/utils.cmake)

add_subdirectory(sources)
add_subdirectory(tests)
