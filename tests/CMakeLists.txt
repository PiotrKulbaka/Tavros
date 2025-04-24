if (TAV_UNITTESTS)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/third/gtest/include)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third/gtest)

    add_executable(tav_test ${CMAKE_CURRENT_LIST_DIR}/math/main.cpp)

    target_link_libraries(
            tav_test
        PRIVATE
            tav_core
            gtest
            gtest_main
    )
endif()
