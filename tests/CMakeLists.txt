
include_directories(${CMAKE_CURRENT_LIST_DIR}/third/gtest/include)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third/gtest)

add_executable(tav_test
    ${CMAKE_CURRENT_LIST_DIR}/math/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/math/vec.cpp
    ${CMAKE_CURRENT_LIST_DIR}/math/common.hpp
)

target_link_libraries(
        tav_test
    PRIVATE
        tav_core
        gtest
)
