#include <common.test.hpp>

#include <tavros/core/math.hpp>

using namespace tavros::math;

class mat4_test : public unittest_scope
{
};


// Test default constructor
TEST_F(mat4_test, default_constructor)
{
    mat4 m;
    EXPECT_EQ(m[0].x, 0.0f);
    EXPECT_EQ(m[0].y, 0.0f);
    EXPECT_EQ(m[0].z, 0.0f);
    EXPECT_EQ(m[0].w, 0.0f);
    EXPECT_EQ(m[1].x, 0.0f);
    EXPECT_EQ(m[1].y, 0.0f);
    EXPECT_EQ(m[1].z, 0.0f);
    EXPECT_EQ(m[1].w, 0.0f);
    EXPECT_EQ(m[2].x, 0.0f);
    EXPECT_EQ(m[2].y, 0.0f);
    EXPECT_EQ(m[2].z, 0.0f);
    EXPECT_EQ(m[2].w, 0.0f);
    EXPECT_EQ(m[3].x, 0.0f);
    EXPECT_EQ(m[3].y, 0.0f);
    EXPECT_EQ(m[3].z, 0.0f);
    EXPECT_EQ(m[3].w, 0.0f);
}

// Test identity method
TEST_F(mat4_test, identity)
{
    mat4 m = mat4::identity();
    EXPECT_EQ(m[0].x, 1.0f);
    EXPECT_EQ(m[0].y, 0.0f);
    EXPECT_EQ(m[0].z, 0.0f);
    EXPECT_EQ(m[0].w, 0.0f);
    EXPECT_EQ(m[1].x, 0.0f);
    EXPECT_EQ(m[1].y, 1.0f);
    EXPECT_EQ(m[1].z, 0.0f);
    EXPECT_EQ(m[1].w, 0.0f);
    EXPECT_EQ(m[2].x, 0.0f);
    EXPECT_EQ(m[2].y, 0.0f);
    EXPECT_EQ(m[2].z, 1.0f);
    EXPECT_EQ(m[2].w, 0.0f);
    EXPECT_EQ(m[3].x, 0.0f);
    EXPECT_EQ(m[3].y, 0.0f);
    EXPECT_EQ(m[3].z, 0.0f);
    EXPECT_EQ(m[3].w, 1.0f);
}

// Test constructor from components
TEST_F(mat4_test, constructor_from_components)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    EXPECT_EQ(m[0].x, 1.0f);
    EXPECT_EQ(m[0].y, 2.0f);
    EXPECT_EQ(m[0].z, 3.0f);
    EXPECT_EQ(m[0].w, 4.0f);
    EXPECT_EQ(m[1].x, 5.0f);
    EXPECT_EQ(m[1].y, 6.0f);
    EXPECT_EQ(m[1].z, 7.0f);
    EXPECT_EQ(m[1].w, 8.0f);
    EXPECT_EQ(m[2].x, 9.0f);
    EXPECT_EQ(m[2].y, 10.0f);
    EXPECT_EQ(m[2].z, 11.0f);
    EXPECT_EQ(m[2].w, 12.0f);
    EXPECT_EQ(m[3].x, 13.0f);
    EXPECT_EQ(m[3].y, 14.0f);
    EXPECT_EQ(m[3].z, 15.0f);
    EXPECT_EQ(m[3].w, 16.0f);
}

// Test constructor from columns
TEST_F(mat4_test, constructor_from_columns)
{
    vec4 col0(1.0f, 5.0f, 9.0f, 13.0f);
    vec4 col1(2.0f, 6.0f, 10.0f, 14.0f);
    vec4 col2(3.0f, 7.0f, 11.0f, 15.0f);
    vec4 col3(4.0f, 8.0f, 12.0f, 16.0f);
    mat4 m(col0, col1, col2, col3);
    EXPECT_EQ(m[0].x, col0.x);
    EXPECT_EQ(m[0].y, col0.y);
    EXPECT_EQ(m[0].z, col0.z);
    EXPECT_EQ(m[0].w, col0.w);
    EXPECT_EQ(m[1].x, col1.x);
    EXPECT_EQ(m[1].y, col1.y);
    EXPECT_EQ(m[1].z, col1.z);
    EXPECT_EQ(m[1].w, col1.w);
    EXPECT_EQ(m[2].x, col2.x);
    EXPECT_EQ(m[2].y, col2.y);
    EXPECT_EQ(m[2].z, col2.z);
    EXPECT_EQ(m[2].w, col2.w);
    EXPECT_EQ(m[3].x, col3.x);
    EXPECT_EQ(m[3].y, col3.y);
    EXPECT_EQ(m[3].z, col3.z);
    EXPECT_EQ(m[3].w, col3.w);
}

// Test constructor from scalar
TEST_F(mat4_test, constructor_from_scalar)
{
    mat4 m(3.0f);
    EXPECT_EQ(m[0].x, 3.0f);
    EXPECT_EQ(m[0].y, 0.0f);
    EXPECT_EQ(m[0].z, 0.0f);
    EXPECT_EQ(m[0].w, 0.0f);
    EXPECT_EQ(m[1].x, 0.0f);
    EXPECT_EQ(m[1].y, 3.0f);
    EXPECT_EQ(m[1].z, 0.0f);
    EXPECT_EQ(m[1].w, 0.0f);
    EXPECT_EQ(m[2].x, 0.0f);
    EXPECT_EQ(m[2].y, 0.0f);
    EXPECT_EQ(m[2].z, 3.0f);
    EXPECT_EQ(m[2].w, 0.0f);
    EXPECT_EQ(m[3].x, 0.0f);
    EXPECT_EQ(m[3].y, 0.0f);
    EXPECT_EQ(m[3].z, 0.0f);
    EXPECT_EQ(m[3].w, 3.0f);
}

// Test operator[] const
TEST_F(mat4_test, operator_bracket_const)
{
    const mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    EXPECT_EQ(m[0].x, 1.0f);
    EXPECT_EQ(m[0].y, 2.0f);
    EXPECT_EQ(m[0].z, 3.0f);
    EXPECT_EQ(m[0].w, 4.0f);
    EXPECT_EQ(m[1].x, 5.0f);
    EXPECT_EQ(m[1].y, 6.0f);
    EXPECT_EQ(m[1].z, 7.0f);
    EXPECT_EQ(m[1].w, 8.0f);
    EXPECT_EQ(m[2].x, 9.0f);
    EXPECT_EQ(m[2].y, 10.0f);
    EXPECT_EQ(m[2].z, 11.0f);
    EXPECT_EQ(m[2].w, 12.0f);
    EXPECT_EQ(m[3].x, 13.0f);
    EXPECT_EQ(m[3].y, 14.0f);
    EXPECT_EQ(m[3].z, 15.0f);
    EXPECT_EQ(m[3].w, 16.0f);
}

TEST_F(mat4_test, operator_bracket_const_assert)
{
    const mat4 m;
    vec4       pad;
    EXPECT_FALSE(assert_was_called());
    auto ub = m[5];
    EXPECT_TRUE(assert_was_called());
}

// Test operator[]
TEST_F(mat4_test, operator_bracket)
{
    mat4 m;
    m[0] = vec4(1.0f, 2.0f, 3.0f, 4.0f);
    m[1] = vec4(5.0f, 6.0f, 7.0f, 8.0f);
    m[2] = vec4(9.0f, 10.0f, 11.0f, 12.0f);
    m[3] = vec4(13.0f, 14.0f, 15.0f, 16.0f);
    EXPECT_EQ(m[0].x, 1.0f);
    EXPECT_EQ(m[0].y, 2.0f);
    EXPECT_EQ(m[0].z, 3.0f);
    EXPECT_EQ(m[0].w, 4.0f);
    EXPECT_EQ(m[1].x, 5.0f);
    EXPECT_EQ(m[1].y, 6.0f);
    EXPECT_EQ(m[1].z, 7.0f);
    EXPECT_EQ(m[1].w, 8.0f);
    EXPECT_EQ(m[2].x, 9.0f);
    EXPECT_EQ(m[2].y, 10.0f);
    EXPECT_EQ(m[2].z, 11.0f);
    EXPECT_EQ(m[2].w, 12.0f);
    EXPECT_EQ(m[3].x, 13.0f);
    EXPECT_EQ(m[3].y, 14.0f);
    EXPECT_EQ(m[3].z, 15.0f);
    EXPECT_EQ(m[3].w, 16.0f);
}

TEST_F(mat4_test, operator_bracket_assert)
{
    mat4 m;
    vec4 pad;
    EXPECT_FALSE(assert_was_called());
    auto ub = m[5];
    EXPECT_TRUE(assert_was_called());
}

// Test operator+=
TEST_F(mat4_test, operator_plus_equals)
{
    mat4 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 m2(16.0f, 15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    m1 += m2;
    EXPECT_EQ(m1[0].x, 17.0f);
    EXPECT_EQ(m1[0].y, 17.0f);
    EXPECT_EQ(m1[0].z, 17.0f);
    EXPECT_EQ(m1[0].w, 17.0f);
    EXPECT_EQ(m1[1].x, 17.0f);
    EXPECT_EQ(m1[1].y, 17.0f);
    EXPECT_EQ(m1[1].z, 17.0f);
    EXPECT_EQ(m1[1].w, 17.0f);
    EXPECT_EQ(m1[2].x, 17.0f);
    EXPECT_EQ(m1[2].y, 17.0f);
    EXPECT_EQ(m1[2].z, 17.0f);
    EXPECT_EQ(m1[2].w, 17.0f);
    EXPECT_EQ(m1[3].x, 17.0f);
    EXPECT_EQ(m1[3].y, 17.0f);
    EXPECT_EQ(m1[3].z, 17.0f);
    EXPECT_EQ(m1[3].w, 17.0f);
}

// Test operator-=
TEST_F(mat4_test, operator_minus_equals)
{
    mat4 m1(16.0f, 15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    mat4 m2(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    m1 -= m2;
    EXPECT_EQ(m1[0].x, 15.0f);
    EXPECT_EQ(m1[0].y, 13.0f);
    EXPECT_EQ(m1[0].z, 11.0f);
    EXPECT_EQ(m1[0].w, 9.0f);
    EXPECT_EQ(m1[1].x, 7.0f);
    EXPECT_EQ(m1[1].y, 5.0f);
    EXPECT_EQ(m1[1].z, 3.0f);
    EXPECT_EQ(m1[1].w, 1.0f);
    EXPECT_EQ(m1[2].x, -1.0f);
    EXPECT_EQ(m1[2].y, -3.0f);
    EXPECT_EQ(m1[2].z, -5.0f);
    EXPECT_EQ(m1[2].w, -7.0f);
    EXPECT_EQ(m1[3].x, -9.0f);
    EXPECT_EQ(m1[3].y, -11.0f);
    EXPECT_EQ(m1[3].z, -13.0f);
    EXPECT_EQ(m1[3].w, -15.0f);
}

// Test operator*=
TEST_F(mat4_test, operator_multiply_equals_matrix)
{
    mat4 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 m2(16.0f, 15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    m1 *= m2;
    EXPECT_EQ(m1[0].x, 386.0f);
    EXPECT_EQ(m1[0].y, 444.0f);
    EXPECT_EQ(m1[0].z, 502.0f);
    EXPECT_EQ(m1[0].w, 560.0f);
    EXPECT_EQ(m1[1].x, 274.0f);
    EXPECT_EQ(m1[1].y, 316.0f);
    EXPECT_EQ(m1[1].z, 358.0f);
    EXPECT_EQ(m1[1].w, 400.0f);
    EXPECT_EQ(m1[2].x, 162.0f);
    EXPECT_EQ(m1[2].y, 188.0f);
    EXPECT_EQ(m1[2].z, 214.0f);
    EXPECT_EQ(m1[2].w, 240.0f);
    EXPECT_EQ(m1[3].x, 50.0f);
    EXPECT_EQ(m1[3].y, 60.0f);
    EXPECT_EQ(m1[3].z, 70.0f);
    EXPECT_EQ(m1[3].w, 80.0f);
}

// Test data() const
TEST_F(mat4_test, data_const)
{
    mat4         m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    const float* data = m.data();
    EXPECT_EQ(data[0], 1.0f);
    EXPECT_EQ(data[1], 2.0f);
    EXPECT_EQ(data[2], 3.0f);
    EXPECT_EQ(data[3], 4.0f);
    EXPECT_EQ(data[4], 5.0f);
    EXPECT_EQ(data[5], 6.0f);
    EXPECT_EQ(data[6], 7.0f);
    EXPECT_EQ(data[7], 8.0f);
    EXPECT_EQ(data[8], 9.0f);
    EXPECT_EQ(data[9], 10.0f);
    EXPECT_EQ(data[10], 11.0f);
    EXPECT_EQ(data[11], 12.0f);
    EXPECT_EQ(data[12], 13.0f);
    EXPECT_EQ(data[13], 14.0f);
    EXPECT_EQ(data[14], 15.0f);
    EXPECT_EQ(data[15], 16.0f);
}

// Test data()
TEST_F(mat4_test, data)
{
    mat4   m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    float* data = m.data();
    EXPECT_EQ(data[0], 1.0f);
    EXPECT_EQ(data[1], 2.0f);
    EXPECT_EQ(data[2], 3.0f);
    EXPECT_EQ(data[3], 4.0f);
    EXPECT_EQ(data[4], 5.0f);
    EXPECT_EQ(data[5], 6.0f);
    EXPECT_EQ(data[6], 7.0f);
    EXPECT_EQ(data[7], 8.0f);
    EXPECT_EQ(data[8], 9.0f);
    EXPECT_EQ(data[9], 10.0f);
    EXPECT_EQ(data[10], 11.0f);
    EXPECT_EQ(data[11], 12.0f);
    EXPECT_EQ(data[12], 13.0f);
    EXPECT_EQ(data[13], 14.0f);
    EXPECT_EQ(data[14], 15.0f);
    EXPECT_EQ(data[15], 16.0f);
}

TEST_F(mat4_test, operator_unary_minus)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 n = -m;
    EXPECT_FLOAT_EQ(n[0].x, -1.0f);
    EXPECT_FLOAT_EQ(n[0].y, -2.0f);
    EXPECT_FLOAT_EQ(n[0].z, -3.0f);
    EXPECT_FLOAT_EQ(n[0].w, -4.0f);
    EXPECT_FLOAT_EQ(n[1].x, -5.0f);
    EXPECT_FLOAT_EQ(n[1].y, -6.0f);
    EXPECT_FLOAT_EQ(n[1].z, -7.0f);
    EXPECT_FLOAT_EQ(n[1].w, -8.0f);
    EXPECT_FLOAT_EQ(n[2].x, -9.0f);
    EXPECT_FLOAT_EQ(n[2].y, -10.0f);
    EXPECT_FLOAT_EQ(n[2].z, -11.0f);
    EXPECT_FLOAT_EQ(n[2].w, -12.0f);
    EXPECT_FLOAT_EQ(n[3].x, -13.0f);
    EXPECT_FLOAT_EQ(n[3].y, -14.0f);
    EXPECT_FLOAT_EQ(n[3].z, -15.0f);
    EXPECT_FLOAT_EQ(n[3].w, -16.0f);
}
TEST_F(mat4_test, operator_plus)
{
    mat4 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 m2(16.0f, 15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    mat4 n = m1 + m2;
    EXPECT_FLOAT_EQ(n[0].x, 17.0f);
    EXPECT_FLOAT_EQ(n[0].y, 17.0f);
    EXPECT_FLOAT_EQ(n[0].z, 17.0f);
    EXPECT_FLOAT_EQ(n[0].w, 17.0f);
    EXPECT_FLOAT_EQ(n[1].x, 17.0f);
    EXPECT_FLOAT_EQ(n[1].y, 17.0f);
    EXPECT_FLOAT_EQ(n[1].z, 17.0f);
    EXPECT_FLOAT_EQ(n[1].w, 17.0f);
    EXPECT_FLOAT_EQ(n[2].x, 17.0f);
    EXPECT_FLOAT_EQ(n[2].y, 17.0f);
    EXPECT_FLOAT_EQ(n[2].z, 17.0f);
    EXPECT_FLOAT_EQ(n[2].w, 17.0f);
    EXPECT_FLOAT_EQ(n[3].x, 17.0f);
    EXPECT_FLOAT_EQ(n[3].y, 17.0f);
    EXPECT_FLOAT_EQ(n[3].z, 17.0f);
    EXPECT_FLOAT_EQ(n[3].w, 17.0f);
}

TEST_F(mat4_test, operator_minus)
{
    mat4 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 m2(16.0f, 15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    mat4 n = m1 - m2;
    EXPECT_FLOAT_EQ(n[0].x, -15.0f);
    EXPECT_FLOAT_EQ(n[0].y, -13.0f);
    EXPECT_FLOAT_EQ(n[0].z, -11.0f);
    EXPECT_FLOAT_EQ(n[0].w, -9.0f);
    EXPECT_FLOAT_EQ(n[1].x, -7.0f);
    EXPECT_FLOAT_EQ(n[1].y, -5.0f);
    EXPECT_FLOAT_EQ(n[1].z, -3.0f);
    EXPECT_FLOAT_EQ(n[1].w, -1.0f);
    EXPECT_FLOAT_EQ(n[2].x, 1.0f);
    EXPECT_FLOAT_EQ(n[2].y, 3.0f);
    EXPECT_FLOAT_EQ(n[2].z, 5.0f);
    EXPECT_FLOAT_EQ(n[2].w, 7.0f);
    EXPECT_FLOAT_EQ(n[3].x, 9.0f);
    EXPECT_FLOAT_EQ(n[3].y, 11.0f);
    EXPECT_FLOAT_EQ(n[3].z, 13.0f);
    EXPECT_FLOAT_EQ(n[3].w, 15.0f);
}
TEST_F(mat4_test, operator_multiply)
{
    mat4 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 m2(16.0f, 15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    mat4 n = m1 * m2;
    EXPECT_FLOAT_EQ(n[0].x, 386.0f);
    EXPECT_FLOAT_EQ(n[0].y, 444.0f);
    EXPECT_FLOAT_EQ(n[0].z, 502.0f);
    EXPECT_FLOAT_EQ(n[0].w, 560.0f);
    EXPECT_FLOAT_EQ(n[1].x, 274.0f);
    EXPECT_FLOAT_EQ(n[1].y, 316.0f);
    EXPECT_FLOAT_EQ(n[1].z, 358.0f);
    EXPECT_FLOAT_EQ(n[1].w, 400.0f);
    EXPECT_FLOAT_EQ(n[2].x, 162.0f);
    EXPECT_FLOAT_EQ(n[2].y, 188.0f);
    EXPECT_FLOAT_EQ(n[2].z, 214.0f);
    EXPECT_FLOAT_EQ(n[2].w, 240.0f);
    EXPECT_FLOAT_EQ(n[3].x, 50.0f);
    EXPECT_FLOAT_EQ(n[3].y, 60.0f);
    EXPECT_FLOAT_EQ(n[3].z, 70.0f);
    EXPECT_FLOAT_EQ(n[3].w, 80.0f);
}

TEST_F(mat4_test, operator_multiply_scalar)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 n = m * 2.0f;
    EXPECT_FLOAT_EQ(n[0].x, 2.0f);
    EXPECT_FLOAT_EQ(n[0].y, 4.0f);
    EXPECT_FLOAT_EQ(n[0].z, 6.0f);
    EXPECT_FLOAT_EQ(n[0].w, 8.0f);
    EXPECT_FLOAT_EQ(n[1].x, 10.0f);
    EXPECT_FLOAT_EQ(n[1].y, 12.0f);
    EXPECT_FLOAT_EQ(n[1].z, 14.0f);
    EXPECT_FLOAT_EQ(n[1].w, 16.0f);
    EXPECT_FLOAT_EQ(n[2].x, 18.0f);
    EXPECT_FLOAT_EQ(n[2].y, 20.0f);
    EXPECT_FLOAT_EQ(n[2].z, 22.0f);
    EXPECT_FLOAT_EQ(n[2].w, 24.0f);
    EXPECT_FLOAT_EQ(n[3].x, 26.0f);
    EXPECT_FLOAT_EQ(n[3].y, 28.0f);
    EXPECT_FLOAT_EQ(n[3].z, 30.0f);
    EXPECT_FLOAT_EQ(n[3].w, 32.0f);
}
TEST_F(mat4_test, operator_scalar_multiply_matrix)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 n = 2.0f * m;
    EXPECT_FLOAT_EQ(n[0].x, 2.0f);
    EXPECT_FLOAT_EQ(n[0].y, 4.0f);
    EXPECT_FLOAT_EQ(n[0].z, 6.0f);
    EXPECT_FLOAT_EQ(n[0].w, 8.0f);
    EXPECT_FLOAT_EQ(n[1].x, 10.0f);
    EXPECT_FLOAT_EQ(n[1].y, 12.0f);
    EXPECT_FLOAT_EQ(n[1].z, 14.0f);
    EXPECT_FLOAT_EQ(n[1].w, 16.0f);
    EXPECT_FLOAT_EQ(n[2].x, 18.0f);
    EXPECT_FLOAT_EQ(n[2].y, 20.0f);
    EXPECT_FLOAT_EQ(n[2].z, 22.0f);
    EXPECT_FLOAT_EQ(n[2].w, 24.0f);
    EXPECT_FLOAT_EQ(n[3].x, 26.0f);
    EXPECT_FLOAT_EQ(n[3].y, 28.0f);
    EXPECT_FLOAT_EQ(n[3].z, 30.0f);
    EXPECT_FLOAT_EQ(n[3].w, 32.0f);
}

TEST_F(mat4_test, operator_divide_scalar)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 n = m / 2.0f;
    EXPECT_FLOAT_EQ(n[0].x, 0.5f);
    EXPECT_FLOAT_EQ(n[0].y, 1.0f);
    EXPECT_FLOAT_EQ(n[0].z, 1.5f);
    EXPECT_FLOAT_EQ(n[0].w, 2.0f);
    EXPECT_FLOAT_EQ(n[1].x, 2.5f);
    EXPECT_FLOAT_EQ(n[1].y, 3.0f);
    EXPECT_FLOAT_EQ(n[1].z, 3.5f);
    EXPECT_FLOAT_EQ(n[1].w, 4.0f);
    EXPECT_FLOAT_EQ(n[2].x, 4.5f);
    EXPECT_FLOAT_EQ(n[2].y, 5.0f);
    EXPECT_FLOAT_EQ(n[2].z, 5.5f);
    EXPECT_FLOAT_EQ(n[2].w, 6.0f);
    EXPECT_FLOAT_EQ(n[3].x, 6.5f);
    EXPECT_FLOAT_EQ(n[3].y, 7.0f);
    EXPECT_FLOAT_EQ(n[3].z, 7.5f);
    EXPECT_FLOAT_EQ(n[3].w, 8.0f);
}
TEST_F(mat4_test, operator_divide_scalar_zero)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 n = m / 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(mat4_test, operator_scalar_divide_matrix)
{
    mat4 m(2.0f, 4.0f, 8.0f, 16.0f, 1.0f, 2.0f, 4.0f, 8.0f, 0.5f, 1.0f, 2.0f, 4.0f, 0.25f, 0.5f, 1.0f, 2.0f);
    mat4 n = 8.0f / m;
    EXPECT_FLOAT_EQ(n[0].x, 4.0f);
    EXPECT_FLOAT_EQ(n[0].y, 2.0f);
    EXPECT_FLOAT_EQ(n[0].z, 1.0f);
    EXPECT_FLOAT_EQ(n[0].w, 0.5f);
    EXPECT_FLOAT_EQ(n[1].x, 8.0f);
    EXPECT_FLOAT_EQ(n[1].y, 4.0f);
    EXPECT_FLOAT_EQ(n[1].z, 2.0f);
    EXPECT_FLOAT_EQ(n[1].w, 1.0f);
    EXPECT_FLOAT_EQ(n[2].x, 16.0f);
    EXPECT_FLOAT_EQ(n[2].y, 8.0f);
    EXPECT_FLOAT_EQ(n[2].z, 4.0f);
    EXPECT_FLOAT_EQ(n[2].w, 2.0f);
    EXPECT_FLOAT_EQ(n[3].x, 32.0f);
    EXPECT_FLOAT_EQ(n[3].y, 16.0f);
    EXPECT_FLOAT_EQ(n[3].z, 8.0f);
    EXPECT_FLOAT_EQ(n[3].w, 4.0f);
}

TEST_F(mat4_test, operator_scalar_divide_matrix_zero)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    EXPECT_FALSE(assert_was_called());
    mat4 n = 0.0f / m;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(mat4_test, almost_equal_nonzero_epsilon)
{
    mat4 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 b(1.0000009f, 2.0000009f, 3.0000009f, 4.0000009f, 5.0000009f, 6.0000009f, 7.0000009f, 8.0000009f, 9.0000009f, 10.0000009f, 11.0000009f, 12.0000009f, 13.0000009f, 14.0000009f, 15.0000009f, 16.0000009f);
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(mat4_test, almost_equal_zero_epsilon)
{
    mat4 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 b(1.1f, 2.1f, 3.1f, 4.1f, 5.1f, 6.1f, 7.1f, 8.1f, 9.1f, 10.1f, 11.1f, 12.1f, 13.1f, 14.1f, 15.1f, 16.1f);
    EXPECT_FALSE(almost_equal(a, b, 0.0f));
}
TEST_F(mat4_test, determinant)
{
    mat4 m(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(determinant(m), 1.0f);
    m[0][0] = 2.0f;
    EXPECT_FLOAT_EQ(determinant(m), 2.0f);
    m[1][1] = 3.0f;
    EXPECT_FLOAT_EQ(determinant(m), 6.0f);
    m[2][2] = 4.0f;
    EXPECT_FLOAT_EQ(determinant(m), 24.0f);
    m[3][3] = 5.0f;
    EXPECT_FLOAT_EQ(determinant(m), 120.0f);
}

TEST_F(mat4_test, inverse)
{
    mat4 m(9.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 1.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 n = inverse(m);
    auto mn = m * n;
    auto nm = n * m;
    EXPECT_TRUE(almost_equal(mn, mat4::identity()));
    EXPECT_TRUE(almost_equal(nm, mat4::identity()));
    EXPECT_FALSE(assert_was_called());
}

TEST_F(mat4_test, inverse_zero_determinant)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 2.0f, 4.0f, 6.0f, 8.0f, 3.0f, 6.0f, 9.0f, 12.0f, 4.0f, 8.0f, 12.0f, 16.0f);
    mat4 n = inverse(m);

    EXPECT_TRUE(almost_zero(n[0][0], k_epsilon6));
    EXPECT_TRUE(almost_zero(n[1][1], k_epsilon6));
    EXPECT_TRUE(almost_zero(n[2][2], k_epsilon6));
    EXPECT_TRUE(almost_zero(n[3][3], k_epsilon6));
    EXPECT_TRUE(assert_was_called());
}

TEST_F(mat4_test, inverse_nonzero_determinant)
{
    mat4 m(9.0f, 2.0f, 3.0f, 4.0f, 4.0f, 5.0f, 6.0f, 7.0f, 7.0f, 11.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f);
    mat4 n = inverse(m);

    EXPECT_FALSE(almost_zero(n[0][0], k_epsilon6));
    EXPECT_FALSE(almost_zero(n[1][1], k_epsilon6));
    EXPECT_FALSE(almost_zero(n[2][2], k_epsilon6));
    EXPECT_FALSE(almost_zero(n[3][3], k_epsilon6));
}

TEST_F(mat4_test, transpose)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
    mat4 n = transpose(m);

    EXPECT_FLOAT_EQ(n[0][0], 1.0f);
    EXPECT_FLOAT_EQ(n[1][0], 2.0f);
    EXPECT_FLOAT_EQ(n[2][0], 3.0f);
    EXPECT_FLOAT_EQ(n[3][0], 4.0f);
    EXPECT_FLOAT_EQ(n[0][1], 5.0f);
    EXPECT_FLOAT_EQ(n[1][1], 6.0f);
    EXPECT_FLOAT_EQ(n[2][1], 7.0f);
    EXPECT_FLOAT_EQ(n[3][1], 8.0f);
    EXPECT_FLOAT_EQ(n[0][2], 9.0f);
    EXPECT_FLOAT_EQ(n[1][2], 10.0f);
    EXPECT_FLOAT_EQ(n[2][2], 11.0f);
    EXPECT_FLOAT_EQ(n[3][2], 12.0f);
    EXPECT_FLOAT_EQ(n[0][3], 13.0f);
    EXPECT_FLOAT_EQ(n[1][3], 14.0f);
    EXPECT_FLOAT_EQ(n[2][3], 15.0f);
    EXPECT_FLOAT_EQ(n[3][3], 16.0f);
}
