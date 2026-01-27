#include <common.test.hpp>

#include <tavros/core/math.hpp>

using namespace tavros::math;

class bitops_test : public unittest_scope
{
};

TEST_F(bitops_test, bit_count_basic)
{
    EXPECT_EQ(bit_count(0b0), 0);
    EXPECT_EQ(bit_count(0b1), 1);
    EXPECT_EQ(bit_count(0b1011), 3);
    EXPECT_EQ(bit_count(UINT64_MAX), 64);
}

TEST_F(bitops_test, is_power_of_two_basic)
{
    EXPECT_FALSE(is_power_of_two(0));
    EXPECT_TRUE(is_power_of_two(1));
    EXPECT_TRUE(is_power_of_two(2));
    EXPECT_TRUE(is_power_of_two(8));
    EXPECT_FALSE(is_power_of_two(10));
    EXPECT_FALSE(is_power_of_two(UINT64_MAX));
}

TEST_F(bitops_test, ceil_power_of_two_basic)
{
    EXPECT_EQ(ceil_power_of_two(0), 1);
    EXPECT_EQ(ceil_power_of_two(1), 1);
    EXPECT_EQ(ceil_power_of_two(5), 8);
    EXPECT_EQ(ceil_power_of_two(16), 16);
    EXPECT_EQ(ceil_power_of_two(31), 32);
}

TEST_F(bitops_test, floor_power_of_two_basic)
{
    EXPECT_EQ(floor_power_of_two(0), 0);
    EXPECT_EQ(floor_power_of_two(1), 1);
    EXPECT_EQ(floor_power_of_two(5), 4);
    EXPECT_EQ(floor_power_of_two(16), 16);
    EXPECT_EQ(floor_power_of_two(31), 16);
}

TEST_F(bitops_test, first_set_bit_basic)
{
    EXPECT_EQ(first_set_bit(0), 64);
    EXPECT_EQ(first_set_bit(1), 0);
    EXPECT_EQ(first_set_bit(0b10010), 1);
    EXPECT_EQ(first_set_bit(0b100000000), 8);
}

TEST_F(bitops_test, first_zero_bit_basic)
{
    EXPECT_EQ(first_zero_bit(UINT64_MAX), 64);
    EXPECT_EQ(first_zero_bit(0), 0);
    EXPECT_EQ(first_zero_bit(0b11110111), 3);
    EXPECT_EQ(first_zero_bit(0b11111101), 1);
}

TEST_F(bitops_test, highest_set_bit_basic)
{
    EXPECT_EQ(highest_set_bit(0), 64);
    EXPECT_EQ(highest_set_bit(1), 0);
    EXPECT_EQ(highest_set_bit(0b10010), 4);
    EXPECT_EQ(highest_set_bit(0b100000000), 8);
}

TEST_F(bitops_test, count_leading_zeros_basic)
{
    EXPECT_EQ(count_leading_zeros(0), 64);
    EXPECT_EQ(count_leading_zeros(1ull << 63), 0);
    EXPECT_EQ(count_leading_zeros(0b1000), 60);
    EXPECT_EQ(count_leading_zeros(0b00010000), 59);
}

TEST_F(bitops_test, count_trailing_zeros_basic)
{
    EXPECT_EQ(count_trailing_zeros(0), 64);
    EXPECT_EQ(count_trailing_zeros(1), 0);
    EXPECT_EQ(count_trailing_zeros(0b1000), 3);
    EXPECT_EQ(count_trailing_zeros(0b00100000), 5);
}
