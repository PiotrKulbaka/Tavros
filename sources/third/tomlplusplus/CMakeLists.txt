cmake_minimum_required(VERSION 3.10)

project(TomlLibrary)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/toml/toml.cpp
    ${CMAKE_CURRENT_LIST_DIR}/toml/toml.hpp
)

add_library(toml STATIC ${SOURCES} ${HEADERS})

target_include_directories(toml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
