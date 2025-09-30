#pragma once

#include <gtest/gtest.h>

extern bool g_tav_enable_utnittest_asserts;
extern bool g_tav_assert_was_called;
extern bool g_stress_skip;

bool assert_was_called();

class unittest_scope : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_tav_assert_was_called = false;
    }

    void TearDown() override
    {
    }
};
