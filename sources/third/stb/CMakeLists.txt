cmake_minimum_required(VERSION 3.10)

project(StbLibrary)

add_library(stb STATIC
    ${CMAKE_CURRENT_LIST_DIR}/stb/stb.c
    ${CMAKE_CURRENT_LIST_DIR}/stb/stb_image.h
    ${CMAKE_CURRENT_LIST_DIR}/stb/stb_image_write.h
    ${CMAKE_CURRENT_LIST_DIR}/stb/stb_truetype.h
    ${CMAKE_CURRENT_LIST_DIR}/stb/stb_rect_pack.h
)

target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
