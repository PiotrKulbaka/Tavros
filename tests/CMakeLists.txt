add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third/gtest)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/sources)

set_target_group(tav_tests "tests")
set_target_group(gtest "tests")
set_target_group(gtest_main "tests")
set_target_group(gmock "tests")
set_target_group(gmock_main "tests")
