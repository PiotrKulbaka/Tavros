include(CMakeSources.txt)

add_executable(tav_tests
    ${TAV_SOURCES_TEST_SOURCES}
)

target_link_libraries(
        tav_tests
    PRIVATE
        tav_core
        gtest
)

target_include_directories(tav_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR})
