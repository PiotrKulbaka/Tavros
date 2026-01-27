#include <common.test.hpp>

#include <tavros/core/math.hpp>

using namespace tavros::math;

class ivec2_test : public unittest_scope
{
};

// Int vector 2D tests

TEST_F(ivec2_test, default_constructor_sets_zero)
{
    ivec2 v;
    EXPECT_EQ(v.x, 0);
    EXPECT_EQ(v.y, 0);
}

TEST_F(ivec2_test, component_constructor_sets_correct_values)
{
    ivec2 v(1, 2);
    EXPECT_EQ(v.x, 1);
    EXPECT_EQ(v.y, 2);
}

TEST_F(ivec2_test, operator_add_assign_works)
{
    ivec2 v1(1, 2);
    ivec2 v2(3, 4);
    v1 += v2;
    EXPECT_EQ(v1.x, 4);
    EXPECT_EQ(v1.y, 6);
}

TEST_F(ivec2_test, operator_sub_assign_works)
{
    ivec2 v1(5, 7);
    ivec2 v2(2, 3);
    v1 -= v2;
    EXPECT_EQ(v1.x, 3);
    EXPECT_EQ(v1.y, 4);
}

TEST_F(ivec2_test, operator_mul_works)
{
    ivec2 v1(2, 3);
    ivec2 v2(4, 5);
    v1 *= v2;
    EXPECT_EQ(v1.x, 8);
    EXPECT_EQ(v1.y, 15);
}

TEST_F(ivec2_test, operator_mul_scalar_works)
{
    ivec2 v(2, 3);
    v *= 2;
    EXPECT_EQ(v.x, 4);
    EXPECT_EQ(v.y, 6);
}

TEST_F(ivec2_test, operator_div_works)
{
    ivec2 v1(4, 16);
    ivec2 v2(2, 4);
    v1 /= v2;
    EXPECT_EQ(v1.x, 2);
    EXPECT_EQ(v1.y, 4);
}

TEST_F(ivec2_test, operator_div_scalar_works)
{
    ivec2 v(4, 8);
    v /= 2;
    EXPECT_EQ(v.x, 2);
    EXPECT_EQ(v.y, 4);
}

TEST_F(ivec2_test, unary_minus_operator_negates_components)
{
    ivec2 v(3, -4);
    ivec2 result = -v;
    EXPECT_EQ(result.x, -3);
    EXPECT_EQ(result.y, 4);
}

TEST_F(ivec2_test, binary_add_operator_adds_components)
{
    ivec2 a(1, 2);
    ivec2 b(3, 4);
    ivec2 result = a + b;
    EXPECT_EQ(result.x, 4);
    EXPECT_EQ(result.y, 6);
}

TEST_F(ivec2_test, binary_sub_operator_subtracts_components)
{
    ivec2 a(5, 6);
    ivec2 b(2, 3);
    ivec2 result = a - b;
    EXPECT_EQ(result.x, 3);
    EXPECT_EQ(result.y, 3);
}

TEST_F(ivec2_test, componentwise_multiplication_works)
{
    ivec2 a(2, 3);
    ivec2 b(4, 5);
    ivec2 result = a * b;
    EXPECT_EQ(result.x, 8);
    EXPECT_EQ(result.y, 15);
}

TEST_F(ivec2_test, scalar_right_multiplication_works)
{
    ivec2 v(2, 3);
    ivec2 result = v * 2;
    EXPECT_EQ(result.x, 4);
    EXPECT_EQ(result.y, 6);
}

TEST_F(ivec2_test, scalar_left_multiplication_works)
{
    ivec2 v(2, 3);
    ivec2 result = 2 * v;
    EXPECT_EQ(result.x, 4);
    EXPECT_EQ(result.y, 6);
}

TEST_F(ivec2_test, componentwise_division_works)
{
    ivec2 a(8, 18);
    ivec2 b(2, 3);
    ivec2 result = a / b;
    EXPECT_EQ(result.x, 4);
    EXPECT_EQ(result.y, 6);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(ivec2_test, scalar_division_works)
{
    ivec2 v(10, 20);
    ivec2 result = v / 2;
    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 10);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(ivec2_test, scalar_divided_by_vector_works)
{
    ivec2 v(2, 4);
    ivec2 result = 8 / v;
    EXPECT_EQ(result.x, 4);
    EXPECT_EQ(result.y, 2);
    EXPECT_FALSE(assert_was_called());
}
