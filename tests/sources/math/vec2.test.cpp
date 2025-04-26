#include <common.test.hpp>

#include <tavros/core/math.hpp>

using namespace tavros::math;

class vec2_test : public unittest_scope
{
};

// Vector 2D tests

TEST_F(vec2_test, default_constructor_sets_zero)
{
    vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST_F(vec2_test, scalar_constructor_sets_both_components)
{
    vec2 v(3.14f);
    EXPECT_FLOAT_EQ(v.x, 3.14f);
    EXPECT_FLOAT_EQ(v.y, 3.14f);
}

TEST_F(vec2_test, component_constructor_sets_correct_values)
{
    vec2 v(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
}

TEST_F(vec2_test, operator_index_const_returns_correct_values)
{
    const vec2 v(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[1], 2.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec2_test, operator_index_assertion_bounds)
{
    const vec2 v;
    float      pad;
    auto       x = v[0];
    auto       y = v[1];
    EXPECT_FALSE(assert_was_called());
    auto ub = v[2];
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec2_test, operator_index_nonconst_modifies_components)
{
    vec2 v;
    v[0] = 5.0f;
    v[1] = 10.0f;
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 10.0f);
}

TEST_F(vec2_test, operator_add_assign_works)
{
    vec2 v1(1.0f, 2.0f);
    vec2 v2(3.0f, 4.0f);
    v1 += v2;
    EXPECT_FLOAT_EQ(v1.x, 4.0f);
    EXPECT_FLOAT_EQ(v1.y, 6.0f);
}

TEST_F(vec2_test, operator_sub_assign_works)
{
    vec2 v1(5.0f, 7.0f);
    vec2 v2(2.0f, 3.0f);
    v1 -= v2;
    EXPECT_FLOAT_EQ(v1.x, 3.0f);
    EXPECT_FLOAT_EQ(v1.y, 4.0f);
}

TEST_F(vec2_test, operator_mul_scalar_works)
{
    vec2 v(2.0f, 3.0f);
    v *= 2.0f;
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 6.0f);
}

TEST_F(vec2_test, operator_div_scalar_works)
{
    vec2 v(4.0f, 8.0f);
    v /= 2.0f;
    EXPECT_FLOAT_EQ(v.x, 2.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
}

TEST_F(vec2_test, operator_div_scalar_zero)
{
    vec2 v(4.0f, 8.0f);
    v /= 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec2_test, operator_div_vector_componentwise_works)
{
    vec2 v(8.0f, 16.0f);
    vec2 divisor(2.0f, 4.0f);
    v /= divisor;
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
}

TEST_F(vec2_test, operator_div_zero_vector_componentwise)
{
    vec2 v(8.0f, 16.0f);
    vec2 divisor(0.0f, 0.0f);
    v /= divisor;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec2_test, data_returns_pointer_to_components)
{
    vec2   v(1.0f, 2.0f);
    float* data = v.data();
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 2.0f);
}

TEST_F(vec2_test, data_const_returns_pointer_to_components)
{
    const vec2   v(1.0f, 2.0f);
    const float* data = v.data();
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 2.0f);
}

TEST_F(vec2_test, unary_minus_operator_negates_components)
{
    vec2 v(3.0f, -4.0f);
    vec2 result = -v;
    EXPECT_FLOAT_EQ(result.x, -3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST_F(vec2_test, binary_add_operator_adds_components)
{
    vec2 a(1.0f, 2.0f);
    vec2 b(3.0f, 4.0f);
    vec2 result = a + b;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST_F(vec2_test, binary_sub_operator_subtracts_components)
{
    vec2 a(5.0f, 6.0f);
    vec2 b(2.0f, 3.0f);
    vec2 result = a - b;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
}

TEST_F(vec2_test, componentwise_multiplication_works)
{
    vec2 a(2.0f, 3.0f);
    vec2 b(4.0f, 5.0f);
    vec2 result = a * b;
    EXPECT_FLOAT_EQ(result.x, 8.0f);
    EXPECT_FLOAT_EQ(result.y, 15.0f);
}

TEST_F(vec2_test, scalar_right_multiplication_works)
{
    vec2 v(2.0f, 3.0f);
    vec2 result = v * 2.0f;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST_F(vec2_test, scalar_left_multiplication_works)
{
    vec2 v(2.0f, 3.0f);
    vec2 result = 2.0f * v;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST_F(vec2_test, componentwise_division_works)
{
    vec2 a(8.0f, 18.0f);
    vec2 b(2.0f, 3.0f);
    vec2 result = a / b;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec2_test, componentwise_division_by_zero_calls_assert)
{
    vec2 a(8.0f, 18.0f);
    vec2 b(0.0f, 1.0f);
    vec2 result = a / b;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec2_test, scalar_division_works)
{
    vec2 v(10.0f, 20.0f);
    vec2 result = v / 2.0f;
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec2_test, scalar_division_by_zero_calls_assert)
{
    vec2 v(10.0f, 20.0f);
    vec2 result = v / 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec2_test, scalar_divided_by_vector_works)
{
    vec2 v(2.0f, 4.0f);
    vec2 result = 8.0f / v;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec2_test, scalar_divided_by_zero_vector_calls_assert)
{
    vec2 v(0.0f, 1.0f);
    vec2 result = 8.0f / v;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec2_test, almost_equal_true_for_close_vectors)
{
    vec2 a(1.00000105f, 2.00000105f);
    vec2 b(1.000002f, 2.000002f);
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(vec2_test, almost_equal_false_for_different_vectors)
{
    vec2 a(1.0f, 2.0f);
    vec2 b(1.0f, 3.0f);
    EXPECT_FALSE(almost_equal(a, b));
}

TEST_F(vec2_test, clamp_inside_bounds_returns_same)
{
    vec2 val(5.0f, 7.0f);
    vec2 minv(1.0f, 2.0f);
    vec2 maxv(10.0f, 20.0f);
    vec2 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 7.0f);
}

TEST_F(vec2_test, clamp_below_min_clamps_to_min)
{
    vec2 val(0.0f, 1.0f);
    vec2 minv(2.0f, 2.0f);
    vec2 maxv(10.0f, 10.0f);
    vec2 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
}

TEST_F(vec2_test, clamp_above_max_clamps_to_max)
{
    vec2 val(12.0f, 15.0f);
    vec2 minv(2.0f, 2.0f);
    vec2 maxv(10.0f, 10.0f);
    vec2 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 10.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
}

TEST_F(vec2_test, cross_magnitude_returns_signed_area)
{
    vec2 a(1.0f, 0.0f);
    vec2 b(0.0f, 1.0f);
    EXPECT_FLOAT_EQ(cross_magnitude(a, b), 1.0f);
}

TEST_F(vec2_test, cross_magnitude_negative_area)
{
    vec2 a(0.0f, 1.0f);
    vec2 b(1.0f, 0.0f);
    EXPECT_FLOAT_EQ(cross_magnitude(a, b), -1.0f);
}

TEST_F(vec2_test, dot_returns_scalar_product)
{
    vec2 a(1.0f, 2.0f);
    vec2 b(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 11.0f);
}

TEST_F(vec2_test, dot_orthogonal_vectors_returns_zero)
{
    vec2 a(1.0f, 0.0f);
    vec2 b(0.0f, 1.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 0.0f);
}

TEST_F(vec2_test, length_returns_correct_value)
{
    vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(length(v), 5.0f);
}

TEST_F(vec2_test, length_zero_vector_is_zero)
{
    vec2 v(0.0f, 0.0f);
    EXPECT_FLOAT_EQ(length(v), 0.0f);
}

TEST_F(vec2_test, lerp_zero_coef_returns_a)
{
    vec2 a(1.0f, 2.0f);
    vec2 b(5.0f, 6.0f);
    vec2 result = lerp(a, b, 0.0f);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
}

TEST_F(vec2_test, lerp_one_coef_returns_b)
{
    vec2 a(1.0f, 2.0f);
    vec2 b(5.0f, 6.0f);
    vec2 result = lerp(a, b, 1.0f);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST_F(vec2_test, lerp_half_coef_interpolates)
{
    vec2 a(0.0f, 0.0f);
    vec2 b(10.0f, 20.0f);
    vec2 result = lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
}

TEST_F(vec2_test, lerp_quater_coef_interpolates)
{
    vec2 a(0.0f, 0.0f);
    vec2 b(10.0f, 20.0f);
    vec2 result = lerp(a, b, 0.25f);
    EXPECT_FLOAT_EQ(result.x, 2.5f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
}

TEST_F(vec2_test, lerp_minus_one_coef_extrapolations)
{
    vec2 a(5.0f, 10.0f);
    vec2 b(10.0f, 20.0f);
    vec2 result = lerp(a, b, -1.0f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST_F(vec2_test, lerp_two_coef_extrapolations)
{
    vec2 a(5.0f, 10.0f);
    vec2 b(10.0f, 20.0f);
    vec2 result = lerp(a, b, 2.0f);
    EXPECT_FLOAT_EQ(result.x, 15.0f);
    EXPECT_FLOAT_EQ(result.y, 30.0f);
}

TEST_F(vec2_test, max_returns_componentwise_max)
{
    vec2 a(1.0f, 5.0f);
    vec2 b(3.0f, 2.0f);
    vec2 result = max(a, b);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
}

TEST_F(vec2_test, min_returns_componentwise_min)
{
    vec2 a(1.0f, 5.0f);
    vec2 b(3.0f, 2.0f);
    vec2 result = min(a, b);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
}

TEST_F(vec2_test, normalize_returns_unit_vector)
{
    vec2  v(3.0f, 4.0f);
    vec2  n = normalize(v);
    float len = length(n);
    EXPECT_NEAR(len, 1.0f, 1e-6f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec2_test, normalize_zero_vector_calls_assert)
{
    vec2 v(0.0f, 0.0f);
    vec2 n = normalize(v);
    EXPECT_TRUE(assert_was_called());
}
