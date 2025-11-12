cmake_minimum_required(VERSION 3.15)

project(zlib C)

set(VERSION "1.3.1")

set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_LIST_DIR}/zlib/zlib.h
)

set(ZLIB_PRIVATE_HDRS
    ${CMAKE_CURRENT_LIST_DIR}/zlib/crc32.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/deflate.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/gzguts.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/inffast.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/inffixed.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/inflate.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/inftrees.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/trees.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/zutil.h
    ${CMAKE_CURRENT_LIST_DIR}/zlib/zconf.h
)
set(ZLIB_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/zlib/adler32.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/compress.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/crc32.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/deflate.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/gzclose.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/gzlib.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/gzread.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/gzwrite.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/inflate.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/infback.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/inftrees.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/inffast.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/trees.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/uncompr.c
    ${CMAKE_CURRENT_LIST_DIR}/zlib/zutil.c
)

add_library(zlibstatic STATIC
    ${ZLIB_SRCS}
    ${ZLIB_PUBLIC_HDRS}
    ${ZLIB_PRIVATE_HDRS}
)

target_include_directories(zlibstatic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
