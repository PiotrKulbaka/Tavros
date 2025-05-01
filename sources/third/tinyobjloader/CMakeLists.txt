cmake_minimum_required(VERSION 3.10)

project(TinyObjLoader)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/tinyobjloader/tiny_obj_loader.cc
    ${CMAKE_CURRENT_LIST_DIR}/tinyobjloader/tiny_obj_loader.h
)

add_library(tinyobjloader STATIC ${SOURCES})

target_include_directories(tinyobjloader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
