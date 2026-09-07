cmake_minimum_required(VERSION 3.10)

project(GladLibrary)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/glad/glad.c
)

set(HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/glad/glad.h
    ${CMAKE_CURRENT_LIST_DIR}/glad/khrplatform.h
)

add_library(glad STATIC ${SOURCES} ${HEADERS})

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
