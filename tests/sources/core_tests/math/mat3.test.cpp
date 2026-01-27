#include <common.test.hpp>

#include <tavros/core/math.hpp>

using namespace tavros::math;

class mat3_test : public unittest_scope
{
};

// Test default constructor
TEST_F(mat3_test, default_constructor)
{
    mat3 m;
    EXPECT_EQ(m[0].x, 0.0f);
    EXPECT_EQ(m[0].y, 0.0f);
    EXPECT_EQ(m[0].z, 0.0f);
    EXPECT_EQ(m[1].x, 0.0f);
    EXPECT_EQ(m[1].y, 0.0f);
    EXPECT_EQ(m[1].z, 0.0f);
    EXPECT_EQ(m[2].x, 0.0f);
    EXPECT_EQ(m[2].y, 0.0f);
    EXPECT_EQ(m[2].z, 0.0f);
}

// Test identity method
TEST_F(mat3_test, identity)
{
    mat3 m = mat3::identity();
    EXPECT_EQ(m[0].x, 1.0f);
    EXPECT_EQ(m[0].y, 0.0f);
    EXPECT_EQ(m[0].z, 0.0f);
    EXPECT_EQ(m[1].x, 0.0f);
    EXPECT_EQ(m[1].y, 1.0f);
    EXPECT_EQ(m[1].z, 0.0f);
    EXPECT_EQ(m[2].x, 0.0f);
    EXPECT_EQ(m[2].y, 0.0f);
    EXPECT_EQ(m[2].z, 1.0f);
}

// Test constructor from components
TEST_F(mat3_test, constructor_from_components)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    EXPECT_EQ(m[0].x, 1.0f);
    EXPECT_EQ(m[0].y, 2.0f);
    EXPECT_EQ(m[0].z, 3.0f);
    EXPECT_EQ(m[1].x, 4.0f);
    EXPECT_EQ(m[1].y, 5.0f);
    EXPECT_EQ(m[1].z, 6.0f);
    EXPECT_EQ(m[2].x, 7.0f);
    EXPECT_EQ(m[2].y, 8.0f);
    EXPECT_EQ(m[2].z, 9.0f);
}

// Test constructor from columns
TEST_F(mat3_test, constructor_from_columns)
{
    vec3 col0(1.0f, 4.0f, 7.0f);
    vec3 col1(2.0f, 5.0f, 8.0f);
    vec3 col2(3.0f, 6.0f, 9.0f);
    mat3 m(col0, col1, col2);
    EXPECT_FLOAT_EQ(m[0].x, col0.x);
    EXPECT_FLOAT_EQ(m[0].y, col0.y);
    EXPECT_FLOAT_EQ(m[0].z, col0.z);
    EXPECT_FLOAT_EQ(m[1].x, col1.x);
    EXPECT_FLOAT_EQ(m[1].y, col1.y);
    EXPECT_FLOAT_EQ(m[1].z, col1.z);
    EXPECT_FLOAT_EQ(m[2].x, col2.x);
    EXPECT_FLOAT_EQ(m[2].y, col2.y);
    EXPECT_FLOAT_EQ(m[2].z, col2.z);
}

// Test constructor from scalar
TEST_F(mat3_test, constructor_crom_scalar)
{
    mat3 m(3.0f);
    EXPECT_FLOAT_EQ(m[0].x, 3.0f);
    EXPECT_FLOAT_EQ(m[0].y, 0.0f);
    EXPECT_FLOAT_EQ(m[0].z, 0.0f);
    EXPECT_FLOAT_EQ(m[1].x, 0.0f);
    EXPECT_FLOAT_EQ(m[1].y, 3.0f);
    EXPECT_FLOAT_EQ(m[1].z, 0.0f);
    EXPECT_FLOAT_EQ(m[2].x, 0.0f);
    EXPECT_FLOAT_EQ(m[2].y, 0.0f);
    EXPECT_FLOAT_EQ(m[2].z, 3.0f);
}

// Test operator[] const
TEST_F(mat3_test, operator_bracket_const)
{
    const mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[0].z, 3.0f);
    EXPECT_FLOAT_EQ(m[1].x, 4.0f);
    EXPECT_FLOAT_EQ(m[1].y, 5.0f);
    EXPECT_FLOAT_EQ(m[1].z, 6.0f);
    EXPECT_FLOAT_EQ(m[2].x, 7.0f);
    EXPECT_FLOAT_EQ(m[2].y, 8.0f);
    EXPECT_FLOAT_EQ(m[2].z, 9.0f);
}

TEST_F(mat3_test, operator_bracket_const_assert)
{
    const mat3 m;
    vec3       pad;
    EXPECT_FALSE(assert_was_called());
    auto ub = m[4];
    EXPECT_TRUE(assert_was_called());
}

// Test operator[]
TEST_F(mat3_test, operator_bracket)
{
    mat3 m;
    m[0] = vec3(1.0f, 2.0f, 3.0f);
    m[1] = vec3(4.0f, 5.0f, 6.0f);
    m[2] = vec3(7.0f, 8.0f, 9.0f);
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[0].z, 3.0f);
    EXPECT_FLOAT_EQ(m[1].x, 4.0f);
    EXPECT_FLOAT_EQ(m[1].y, 5.0f);
    EXPECT_FLOAT_EQ(m[1].z, 6.0f);
    EXPECT_FLOAT_EQ(m[2].x, 7.0f);
    EXPECT_FLOAT_EQ(m[2].y, 8.0f);
    EXPECT_FLOAT_EQ(m[2].z, 9.0f);
}

TEST_F(mat3_test, operator_bracket_assert)
{
    mat3 m;
    vec3 pad;
    EXPECT_FALSE(assert_was_called());
    auto ub = m[4];
    EXPECT_TRUE(assert_was_called());
}

// Test operator+=
TEST_F(mat3_test, operator_plus_equals)
{
    mat3 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 m2(9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    m1 += m2;
    EXPECT_FLOAT_EQ(m1[0].x, 10.0f);
    EXPECT_FLOAT_EQ(m1[0].y, 10.0f);
    EXPECT_FLOAT_EQ(m1[0].z, 10.0f);
    EXPECT_FLOAT_EQ(m1[1].x, 10.0f);
    EXPECT_FLOAT_EQ(m1[1].y, 10.0f);
    EXPECT_FLOAT_EQ(m1[1].z, 10.0f);
    EXPECT_FLOAT_EQ(m1[2].x, 10.0f);
    EXPECT_FLOAT_EQ(m1[2].y, 10.0f);
    EXPECT_FLOAT_EQ(m1[2].z, 10.0f);
}

// Test operator-=
TEST_F(mat3_test, operator_minus_equals)
{
    mat3 m1(9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    mat3 m2(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    m1 -= m2;
    EXPECT_FLOAT_EQ(m1[0].x, 8.0f);
    EXPECT_FLOAT_EQ(m1[0].y, 6.0f);
    EXPECT_FLOAT_EQ(m1[0].z, 4.0f);
    EXPECT_FLOAT_EQ(m1[1].x, 2.0f);
    EXPECT_FLOAT_EQ(m1[1].y, 0.0f);
    EXPECT_FLOAT_EQ(m1[1].z, -2.0f);
    EXPECT_FLOAT_EQ(m1[2].x, -4.0f);
    EXPECT_FLOAT_EQ(m1[2].y, -6.0f);
    EXPECT_FLOAT_EQ(m1[2].z, -8.0f);
}

// Test operator*=
TEST_F(mat3_test, operator_multiply_equals_matrix)
{
    mat3 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 m2(9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    m1 *= m2;
    EXPECT_FLOAT_EQ(m1[0].x, 90.0f);
    EXPECT_FLOAT_EQ(m1[0].y, 114.0f);
    EXPECT_FLOAT_EQ(m1[0].z, 138.0f);
    EXPECT_FLOAT_EQ(m1[1].x, 54.0f);
    EXPECT_FLOAT_EQ(m1[1].y, 69.0f);
    EXPECT_FLOAT_EQ(m1[1].z, 84.0f);
    EXPECT_FLOAT_EQ(m1[2].x, 18.0f);
    EXPECT_FLOAT_EQ(m1[2].y, 24.0f);
    EXPECT_FLOAT_EQ(m1[2].z, 30.0f);
}

// Test operator*=
TEST_F(mat3_test, operator_multipl_equals_scalar)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    m *= 2.0f;
    EXPECT_FLOAT_EQ(m[0].x, 2.0f);
    EXPECT_FLOAT_EQ(m[0].y, 4.0f);
    EXPECT_FLOAT_EQ(m[0].z, 6.0f);
    EXPECT_FLOAT_EQ(m[1].x, 8.0f);
    EXPECT_FLOAT_EQ(m[1].y, 10.0f);
    EXPECT_FLOAT_EQ(m[1].z, 12.0f);
    EXPECT_FLOAT_EQ(m[2].x, 14.0f);
    EXPECT_FLOAT_EQ(m[2].y, 16.0f);
    EXPECT_FLOAT_EQ(m[2].z, 18.0f);
}

// Test data() const
TEST_F(mat3_test, data_const)
{
    mat3         m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
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
}

// Test data()
TEST_F(mat3_test, data)
{
    mat3   m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
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
}

TEST_F(mat3_test, operator_unary_minus)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 n = -m;
    EXPECT_FLOAT_EQ(n[0].x, -1.0f);
    EXPECT_FLOAT_EQ(n[0].y, -2.0f);
    EXPECT_FLOAT_EQ(n[0].z, -3.0f);
    EXPECT_FLOAT_EQ(n[1].x, -4.0f);
    EXPECT_FLOAT_EQ(n[1].y, -5.0f);
    EXPECT_FLOAT_EQ(n[1].z, -6.0f);
    EXPECT_FLOAT_EQ(n[2].x, -7.0f);
    EXPECT_FLOAT_EQ(n[2].y, -8.0f);
    EXPECT_FLOAT_EQ(n[2].z, -9.0f);
}

TEST_F(mat3_test, operator_plus)
{
    mat3 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 m2(9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    mat3 n = m1 + m2;
    EXPECT_FLOAT_EQ(n[0].x, 10.0f);
    EXPECT_FLOAT_EQ(n[0].y, 10.0f);
    EXPECT_FLOAT_EQ(n[0].z, 10.0f);
    EXPECT_FLOAT_EQ(n[1].x, 10.0f);
    EXPECT_FLOAT_EQ(n[1].y, 10.0f);
    EXPECT_FLOAT_EQ(n[1].z, 10.0f);
    EXPECT_FLOAT_EQ(n[2].x, 10.0f);
    EXPECT_FLOAT_EQ(n[2].y, 10.0f);
    EXPECT_FLOAT_EQ(n[2].z, 10.0f);
}

TEST_F(mat3_test, operator_minus)
{
    mat3 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 m2(9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    mat3 n = m1 - m2;
    EXPECT_FLOAT_EQ(n[0].x, -8.0f);
    EXPECT_FLOAT_EQ(n[0].y, -6.0f);
    EXPECT_FLOAT_EQ(n[0].z, -4.0f);
    EXPECT_FLOAT_EQ(n[1].x, -2.0f);
    EXPECT_FLOAT_EQ(n[1].y, 0.0f);
    EXPECT_FLOAT_EQ(n[1].z, 2.0f);
    EXPECT_FLOAT_EQ(n[2].x, 4.0f);
    EXPECT_FLOAT_EQ(n[2].y, 6.0f);
    EXPECT_FLOAT_EQ(n[2].z, 8.0f);
}

TEST_F(mat3_test, operator_multiply)
{
    mat3 m1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 m2(9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    mat3 n = m1 * m2;
    EXPECT_FLOAT_EQ(n[0].x, 90.0f);
    EXPECT_FLOAT_EQ(n[0].y, 114.0f);
    EXPECT_FLOAT_EQ(n[0].z, 138.0f);
    EXPECT_FLOAT_EQ(n[1].x, 54.0f);
    EXPECT_FLOAT_EQ(n[1].y, 69.0f);
    EXPECT_FLOAT_EQ(n[1].z, 84.0f);
    EXPECT_FLOAT_EQ(n[2].x, 18.0f);
    EXPECT_FLOAT_EQ(n[2].y, 24.0f);
    EXPECT_FLOAT_EQ(n[2].z, 30.0f);
}

TEST_F(mat3_test, operator_multiply_scalar)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 n = m * 2.0f;
    EXPECT_FLOAT_EQ(n[0].x, 2.0f);
    EXPECT_FLOAT_EQ(n[0].y, 4.0f);
    EXPECT_FLOAT_EQ(n[0].z, 6.0f);
    EXPECT_FLOAT_EQ(n[1].x, 8.0f);
    EXPECT_FLOAT_EQ(n[1].y, 10.0f);
    EXPECT_FLOAT_EQ(n[1].z, 12.0f);
    EXPECT_FLOAT_EQ(n[2].x, 14.0f);
    EXPECT_FLOAT_EQ(n[2].y, 16.0f);
    EXPECT_FLOAT_EQ(n[2].z, 18.0f);
}

TEST_F(mat3_test, operator_scalar_multiply_matrix)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 n = 2.0f * m;
    EXPECT_FLOAT_EQ(n[0].x, 2.0f);
    EXPECT_FLOAT_EQ(n[0].y, 4.0f);
    EXPECT_FLOAT_EQ(n[0].z, 6.0f);
    EXPECT_FLOAT_EQ(n[1].x, 8.0f);
    EXPECT_FLOAT_EQ(n[1].y, 10.0f);
    EXPECT_FLOAT_EQ(n[1].z, 12.0f);
    EXPECT_FLOAT_EQ(n[2].x, 14.0f);
    EXPECT_FLOAT_EQ(n[2].y, 16.0f);
    EXPECT_FLOAT_EQ(n[2].z, 18.0f);
}

TEST_F(mat3_test, operator_divide_scalar)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 n = m / 2.0f;
    EXPECT_FLOAT_EQ(n[0].x, 0.5f);
    EXPECT_FLOAT_EQ(n[0].y, 1.0f);
    EXPECT_FLOAT_EQ(n[0].z, 1.5f);
    EXPECT_FLOAT_EQ(n[1].x, 2.0f);
    EXPECT_FLOAT_EQ(n[1].y, 2.5f);
    EXPECT_FLOAT_EQ(n[1].z, 3.0f);
    EXPECT_FLOAT_EQ(n[2].x, 3.5f);
    EXPECT_FLOAT_EQ(n[2].y, 4.0f);
    EXPECT_FLOAT_EQ(n[2].z, 4.5f);
}

TEST_F(mat3_test, operator_divide_scalar_zero)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 n = m / 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(mat3_test, operator_scalar_divide_matrix)
{
    mat3 m(2.0f, 4.0f, 8.0f, 1.0f, 2.0f, 4.0f, 0.5f, 1.0f, 2.0f);
    mat3 n = 8.0f / m;
    EXPECT_FLOAT_EQ(n[0].x, 4.0f);
    EXPECT_FLOAT_EQ(n[0].y, 2.0f);
    EXPECT_FLOAT_EQ(n[0].z, 1.0f);
    EXPECT_FLOAT_EQ(n[1].x, 8.0f);
    EXPECT_FLOAT_EQ(n[1].y, 4.0f);
    EXPECT_FLOAT_EQ(n[1].z, 2.0f);
    EXPECT_FLOAT_EQ(n[2].x, 16.0f);
    EXPECT_FLOAT_EQ(n[2].y, 8.0f);
    EXPECT_FLOAT_EQ(n[2].z, 4.0f);
}

TEST_F(mat3_test, operator_scalar_divide_matrix_zero)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.0f, 7.0f, 8.0f, 9.0f);
    EXPECT_FALSE(assert_was_called());
    mat3 n = 0.0f / m;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(mat3_test, almost_equal_nonzero_epsilon)
{
    mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 b(1.000001f, 2.000001f, 3.000001f, 4.000001f, 5.000001f, 6.000001f, 7.000001f, 8.000001f, 9.000001f);
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(mat3_test, almost_equal_zero_epsilon)
{
    mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 b(1.1f, 2.1f, 3.1f, 4.1f, 5.1f, 6.1f, 7.1f, 8.1f, 9.1f);
    EXPECT_FALSE(almost_equal(a, b, 0.0f));
}

TEST_F(mat3_test, determinant)
{
    mat3 m(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(determinant(m), 1.0f);
    m[0][0] = 2.0f;
    EXPECT_FLOAT_EQ(determinant(m), 2.0f);
    m[1][1] = 3.0f;
    EXPECT_FLOAT_EQ(determinant(m), 6.0f);
    m[2][2] = 4.0f;
    EXPECT_FLOAT_EQ(determinant(m), 24.0f);
}

TEST_F(mat3_test, inverse)
{
    mat3 m(9.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 n = inverse(m);

    EXPECT_TRUE(almost_equal(m * n, mat3::identity()));
    EXPECT_TRUE(almost_equal(n * m, mat3::identity()));
    EXPECT_FALSE(assert_was_called());
}

TEST_F(mat3_test, inverse_zero_determinant)
{
    mat3 m(1.0f, 2.0f, 3.0f, 2.0f, 4.0f, 6.0f, 3.0f, 6.0f, 9.0f);
    mat3 n = inverse(m);

    EXPECT_TRUE(almost_zero(n[0][0], k_epsilon6));
    EXPECT_TRUE(almost_zero(n[1][1], k_epsilon6));
    EXPECT_TRUE(almost_zero(n[2][2], k_epsilon6));
    EXPECT_TRUE(assert_was_called());
}

TEST_F(mat3_test, inverse_nonzero_determinant)
{
    mat3 m(9.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 n = inverse(m);

    EXPECT_FALSE(almost_zero(n[0][0], k_epsilon6));
    EXPECT_FALSE(almost_zero(n[1][1], k_epsilon6));
    EXPECT_FALSE(almost_zero(n[2][2], k_epsilon6));
}

TEST_F(mat3_test, transpose)
{
    mat3 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    mat3 n = transpose(m);

    EXPECT_FLOAT_EQ(n[0][0], 1.0f);
    EXPECT_FLOAT_EQ(n[1][0], 2.0f);
    EXPECT_FLOAT_EQ(n[2][0], 3.0f);
    EXPECT_FLOAT_EQ(n[0][1], 4.0f);
    EXPECT_FLOAT_EQ(n[1][1], 5.0f);
    EXPECT_FLOAT_EQ(n[2][1], 6.0f);
    EXPECT_FLOAT_EQ(n[0][2], 7.0f);
    EXPECT_FLOAT_EQ(n[1][2], 8.0f);
    EXPECT_FLOAT_EQ(n[2][2], 9.0f);
}
