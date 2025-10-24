cmake_minimum_required(VERSION 3.10)

project(StbLibrary)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/stb/stb.c
)

set(HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/stb_image.h
    ${CMAKE_CURRENT_LIST_DIR}/stb_image_write.h
    ${CMAKE_CURRENT_LIST_DIR}/stb_truetype.h
)

add_library(stb STATIC ${SOURCES})

target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
