#include <gtest/gtest.h>

#include "common.hpp"

bool assert_was_called()
{
    auto ret = g_tav_assert_was_called;
    g_tav_assert_was_called = false;
    return ret;
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    g_tav_enable_utnittest_asserts = true;

    return RUN_ALL_TESTS();
}
