cmake_minimum_required(VERSION 3.10)

project(FastHalfFloat)

add_library(fhf STATIC 
    ${CMAKE_CURRENT_LIST_DIR}/fhf/fhf.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fhf/fhf.hpp
)

target_include_directories(fhf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
