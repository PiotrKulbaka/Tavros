#include <common.test.hpp>

#include <tavros/core/math.hpp>

using namespace tavros::math;

class euler3_test : public unittest_scope
{
};

TEST_F(euler3_test, default_constructor_works)
{
    euler3 e;
    EXPECT_EQ(e[0], 0.0f);
    EXPECT_EQ(e[1], 0.0f);
    EXPECT_EQ(e[2], 0.0f);
}

TEST_F(euler3_test, constructor_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(e.roll, 1.0f);
    EXPECT_FLOAT_EQ(e.pitch, 2.0f);
    EXPECT_FLOAT_EQ(e.yaw, 3.0f);
}

TEST_F(euler3_test, constructor_from_vec3_works)
{
    euler3 e(vec3(1.0f, 2.0f, 3.0f));
    EXPECT_FLOAT_EQ(e.roll, 1.0f);
    EXPECT_FLOAT_EQ(e.pitch, 2.0f);
    EXPECT_FLOAT_EQ(e.yaw, 3.0f);
}

TEST_F(euler3_test, operator_bracket_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(e[0], 1.0f);
    EXPECT_FLOAT_EQ(e[1], 2.0f);
    EXPECT_FLOAT_EQ(e[2], 3.0f);
}

TEST_F(euler3_test, operator_bracket_assets_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    float  pad;
    EXPECT_FALSE(assert_was_called());
    auto ub = e[3];
    EXPECT_TRUE(assert_was_called());
}

TEST_F(euler3_test, operator_bracket_const_works)
{
    const euler3 e(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(e[0], 1.0f);
    EXPECT_FLOAT_EQ(e[1], 2.0f);
    EXPECT_FLOAT_EQ(e[2], 3.0f);
}

TEST_F(euler3_test, operator_bracket_const_assets_works)
{
    const euler3 e(1.0f, 2.0f, 3.0f);
    float        pad;
    EXPECT_FALSE(assert_was_called());
    auto ub = e[3];
    EXPECT_TRUE(assert_was_called());
}

TEST_F(euler3_test, operator_plus_assign_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    euler3 other(4.0f, 5.0f, 6.0f);
    e += other;
    EXPECT_FLOAT_EQ(e.roll, 5.0f);
    EXPECT_FLOAT_EQ(e.pitch, 7.0f);
    EXPECT_FLOAT_EQ(e.yaw, 9.0f);
}

TEST_F(euler3_test, operator_minus_assign_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    euler3 other(4.0f, 5.0f, 6.0f);
    e -= other;
    EXPECT_FLOAT_EQ(e.roll, -3.0f);
    EXPECT_FLOAT_EQ(e.pitch, -3.0f);
    EXPECT_FLOAT_EQ(e.yaw, -3.0f);
}

TEST_F(euler3_test, operator_times_assign_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    e *= 2.0f;
    EXPECT_FLOAT_EQ(e.roll, 2.0f);
    EXPECT_FLOAT_EQ(e.pitch, 4.0f);
    EXPECT_FLOAT_EQ(e.yaw, 6.0f);
}

TEST_F(euler3_test, data_works)
{
    euler3       e(1.0f, 2.0f, 3.0f);
    const float* data = e.data();
    EXPECT_EQ(data[0], 1.0f);
    EXPECT_EQ(data[1], 2.0f);
    EXPECT_EQ(data[2], 3.0f);
}

TEST_F(euler3_test, data_const_works)
{
    const euler3 e(1.0f, 2.0f, 3.0f);
    const float* data = e.data();
    EXPECT_EQ(data[0], 1.0f);
    EXPECT_EQ(data[1], 2.0f);
    EXPECT_EQ(data[2], 3.0f);
}

TEST_F(euler3_test, operator_unary_minus_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    euler3 result = -e;
    EXPECT_FLOAT_EQ(result.roll, -1.0f);
    EXPECT_FLOAT_EQ(result.pitch, -2.0f);
    EXPECT_FLOAT_EQ(result.yaw, -3.0f);
}

TEST_F(euler3_test, operator_plus_works)
{
    euler3 a(1.0f, 2.0f, 3.0f);
    euler3 b(4.0f, 5.0f, 6.0f);
    euler3 result = a + b;
    EXPECT_FLOAT_EQ(result.roll, 5.0f);
    EXPECT_FLOAT_EQ(result.pitch, 7.0f);
    EXPECT_FLOAT_EQ(result.yaw, 9.0f);
}

TEST_F(euler3_test, operator_minus_works)
{
    euler3 a(4.0f, 5.0f, 6.0f);
    euler3 b(1.0f, 2.0f, 3.0f);
    euler3 result = a - b;
    EXPECT_FLOAT_EQ(result.roll, 3.0f);
    EXPECT_FLOAT_EQ(result.pitch, 3.0f);
    EXPECT_FLOAT_EQ(result.yaw, 3.0f);
}

TEST_F(euler3_test, operator_multiply_euler_works)
{
    euler3 a(1.0f, 2.0f, 3.0f);
    euler3 b(4.0f, 5.0f, 6.0f);
    euler3 result = a * b;
    EXPECT_FLOAT_EQ(result.roll, a.roll * b.roll);
    EXPECT_FLOAT_EQ(result.pitch, a.pitch * b.pitch);
    EXPECT_FLOAT_EQ(result.yaw, a.yaw * b.yaw);
}

TEST_F(euler3_test, operator_multiply_scalar_right_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    euler3 result = e * 2.0f;
    EXPECT_FLOAT_EQ(result.roll, 2.0f);
    EXPECT_FLOAT_EQ(result.pitch, 4.0f);
    EXPECT_FLOAT_EQ(result.yaw, 6.0f);
}

TEST_F(euler3_test, operator_multiply_scalar_left_works)
{
    euler3 e(1.0f, 2.0f, 3.0f);
    euler3 result = 2.0f * e;
    EXPECT_FLOAT_EQ(result.roll, 2.0f);
    EXPECT_FLOAT_EQ(result.pitch, 4.0f);
    EXPECT_FLOAT_EQ(result.yaw, 6.0f);
}

TEST_F(euler3_test, operator_divide_euler_works)
{
    euler3 a(4.0f, 6.0f, 8.0f);
    euler3 b(2.0f, 3.0f, 4.0f);
    euler3 result = a / b;
    EXPECT_FLOAT_EQ(result.roll, a.roll / b.roll);
    EXPECT_FLOAT_EQ(result.pitch, a.pitch / b.pitch);
    EXPECT_FLOAT_EQ(result.yaw, a.yaw / b.yaw);
}

TEST_F(euler3_test, operator_divide_scalar_right_works)
{
    euler3 e(4.0f, 6.0f, 8.0f);
    euler3 result = e / 2.0f;
    EXPECT_FLOAT_EQ(result.roll, 2.0f);
    EXPECT_FLOAT_EQ(result.pitch, 3.0f);
    EXPECT_FLOAT_EQ(result.yaw, 4.0f);
}

TEST_F(euler3_test, operator_divide_scalar_left_works)
{
    euler3 e(1.0f, 2.0f, 4.0f);
    euler3 result = 8.0f / e;
    EXPECT_FLOAT_EQ(result.roll, 8.0f / e.roll);
    EXPECT_FLOAT_EQ(result.pitch, 8.0f / e.pitch);
    EXPECT_FLOAT_EQ(result.yaw, 8.0f / e.yaw);
}
