#include <common.test.hpp>

#include <tavros/core/math.hpp>

using namespace tavros::math;

class vec4_test : public unittest_scope
{
};

TEST_F(vec4_test, default_constructor_sets_zero)
{
    vec4 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
    EXPECT_FLOAT_EQ(v.w, 0.0f);
}

TEST_F(vec4_test, scalar_constructor_sets_all_components)
{
    vec4 v(3.14f);
    EXPECT_FLOAT_EQ(v.x, 3.14f);
    EXPECT_FLOAT_EQ(v.y, 3.14f);
    EXPECT_FLOAT_EQ(v.z, 3.14f);
    EXPECT_FLOAT_EQ(v.w, 3.14f);
}

TEST_F(vec4_test, component_constructor_sets_correct_values)
{
    vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
    EXPECT_FLOAT_EQ(v.w, 4.0f);
}

TEST_F(vec4_test, operator_index_const_returns_correct_values)
{
    const vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[1], 2.0f);
    EXPECT_FLOAT_EQ(v[2], 3.0f);
    EXPECT_FLOAT_EQ(v[3], 4.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec4_test, operator_index_assertion_bounds)
{
    const vec4 v;
    float      pad;
    auto       x = v[0];
    auto       y = v[1];
    auto       z = v[2];
    auto       w = v[3];
    EXPECT_FALSE(assert_was_called());
    auto ub = v[4];
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec4_test, operator_index_nonconst_modifies_components)
{
    vec4 v;
    v[0] = 5.0f;
    v[1] = 10.0f;
    v[2] = 15.0f;
    v[3] = 20.0f;
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 10.0f);
    EXPECT_FLOAT_EQ(v.z, 15.0f);
    EXPECT_FLOAT_EQ(v.w, 20.0f);
}

TEST_F(vec4_test, operator_add_assign_works)
{
    vec4 v1(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 v2(5.0f, 6.0f, 7.0f, 8.0f);
    v1 += v2;
    EXPECT_FLOAT_EQ(v1.x, 6.0f);
    EXPECT_FLOAT_EQ(v1.y, 8.0f);
    EXPECT_FLOAT_EQ(v1.z, 10.0f);
    EXPECT_FLOAT_EQ(v1.w, 12.0f);
}

TEST_F(vec4_test, operator_sub_assign_works)
{
    vec4 v1(5.0f, 7.0f, 9.0f, 11.0f);
    vec4 v2(2.0f, 3.0f, 4.0f, 5.0f);
    v1 -= v2;
    EXPECT_FLOAT_EQ(v1.x, 3.0f);
    EXPECT_FLOAT_EQ(v1.y, 4.0f);
    EXPECT_FLOAT_EQ(v1.z, 5.0f);
    EXPECT_FLOAT_EQ(v1.w, 6.0f);
}

TEST_F(vec4_test, operator_mul_scalar_works)
{
    vec4 v(2.0f, 3.0f, 4.0f, 5.0f);
    v *= 2.0f;
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 6.0f);
    EXPECT_FLOAT_EQ(v.z, 8.0f);
    EXPECT_FLOAT_EQ(v.w, 10.0f);
}

TEST_F(vec4_test, operator_div_scalar_works)
{
    vec4 v(4.0f, 8.0f, 12.0f, 16.0f);
    v /= 2.0f;
    EXPECT_FLOAT_EQ(v.x, 2.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
    EXPECT_FLOAT_EQ(v.z, 6.0f);
    EXPECT_FLOAT_EQ(v.w, 8.0f);
}

TEST_F(vec4_test, operator_div_scalar_zero)
{
    vec4 v(4.0f, 8.0f, 12.0f, 16.0f);
    v /= 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec4_test, operator_div_vector_componentwise_works)
{
    vec4 v(8.0f, 16.0f, 24.0f, 32.0f);
    vec4 divisor(2.0f, 4.0f, 6.0f, 8.0f);
    v /= divisor;
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
    EXPECT_FLOAT_EQ(v.z, 4.0f);
    EXPECT_FLOAT_EQ(v.w, 4.0f);
}

TEST_F(vec4_test, operator_div_zero_vector_componentwise)
{
    vec4 v(8.0f, 16.0f, 24.0f, 32.0f);
    vec4 divisor(0.0f, 0.0f, 0.0f, 0.0f);
    v /= divisor;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec4_test, data_returns_pointer_to_components)
{
    vec4   v(1.0f, 2.0f, 3.0f, 4.0f);
    float* data = v.data();
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 2.0f);
    EXPECT_FLOAT_EQ(data[2], 3.0f);
    EXPECT_FLOAT_EQ(data[3], 4.0f);
}

TEST_F(vec4_test, data_const_returns_pointer_to_components)
{
    const vec4   v(1.0f, 2.0f, 3.0f, 4.0f);
    const float* data = v.data();
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 2.0f);
    EXPECT_FLOAT_EQ(data[2], 3.0f);
    EXPECT_FLOAT_EQ(data[3], 4.0f);
}

TEST_F(vec4_test, unary_minus_operator_negates_components)
{
    vec4 v(3.0f, -4.0f, 5.0f, -6.0f);
    vec4 result = -v;
    EXPECT_FLOAT_EQ(result.x, -3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, -5.0f);
    EXPECT_FLOAT_EQ(result.w, 6.0f);
}

TEST_F(vec4_test, binary_add_operator_adds_components)
{
    vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    vec4 result = a + b;
    EXPECT_FLOAT_EQ(result.x, 6.0f);
    EXPECT_FLOAT_EQ(result.y, 8.0f);
    EXPECT_FLOAT_EQ(result.z, 10.0f);
    EXPECT_FLOAT_EQ(result.w, 12.0f);
}

TEST_F(vec4_test, binary_sub_operator_subtracts_components)
{
    vec4 a(5.0f, 7.0f, 9.0f, 11.0f);
    vec4 b(2.0f, 3.0f, 4.0f, 5.0f);
    vec4 result = a - b;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, 5.0f);
    EXPECT_FLOAT_EQ(result.w, 6.0f);
}

TEST_F(vec4_test, componentwise_multiplication_works)
{
    vec4 a(2.0f, 3.0f, 4.0f, 5.0f);
    vec4 b(6.0f, 8.0f, 10.0f, 12.0f);
    vec4 result = a * b;
    EXPECT_FLOAT_EQ(result.x, 12.0f);
    EXPECT_FLOAT_EQ(result.y, 24.0f);
    EXPECT_FLOAT_EQ(result.z, 40.0f);
    EXPECT_FLOAT_EQ(result.w, 60.0f);
}

TEST_F(vec4_test, scalar_right_multiplication_works)
{
    vec4 v(2.0f, 3.0f, 4.0f, 5.0f);
    vec4 result = v * 2.0f;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FLOAT_EQ(result.z, 8.0f);
    EXPECT_FLOAT_EQ(result.w, 10.0f);
}

TEST_F(vec4_test, scalar_left_multiplication_works)
{
    vec4 v(2.0f, 3.0f, 4.0f, 5.0f);
    vec4 result = 2.0f * v;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FLOAT_EQ(result.z, 8.0f);
    EXPECT_FLOAT_EQ(result.w, 10.0f);
}

TEST_F(vec4_test, componentwise_division_works)
{
    vec4 a(8.0f, 18.0f, 32.0f, 60.0f);
    vec4 b(2.0f, 3.0f, 4.0f, 5.0f);
    vec4 result = a / b;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FLOAT_EQ(result.z, 8.0f);
    EXPECT_FLOAT_EQ(result.w, 12.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec4_test, componentwise_division_by_zero_calls_assert)
{
    vec4 a(8.0f, 18.0f, 24.0f, 30.0f);
    vec4 b(0.0f, 1.0f, 2.0f, 3.0f);
    vec4 result = a / b;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec4_test, scalar_division_works)
{
    vec4 v(10.0f, 20.0f, 30.0f, 40.0f);
    vec4 result = v / 2.0f;
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FLOAT_EQ(result.z, 15.0f);
    EXPECT_FLOAT_EQ(result.w, 20.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec4_test, scalar_division_by_zero_calls_assert)
{
    vec4 v(10.0f, 20.0f, 30.0f, 40.0f);
    vec4 result = v / 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec4_test, scalar_divided_by_vector_works)
{
    vec4 v(2.0f, 4.0f, 8.0f, 16.0f);
    vec4 result = 16.0f / v;
    EXPECT_FLOAT_EQ(result.x, 8.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, 2.0f);
    EXPECT_FLOAT_EQ(result.w, 1.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec4_test, scalar_divided_by_zero_vector_calls_assert)
{
    vec4 v(0.0f, 1.0f, 2.0f, 3.0f);
    vec4 result = 8.0f / v;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec4_test, almost_equal_true_for_close_vectors)
{
    vec4 a(1.00000105f, 2.00000105f, 3.00000105f, 4.00000105f);
    vec4 b(1.000002f, 2.000002f, 3.000002f, 4.000002f);
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(vec4_test, almost_equal_false_for_different_vectors)
{
    vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 b(1.0f, 2.0f, 3.0f, 5.0f);
    EXPECT_FALSE(almost_equal(a, b));
}

TEST_F(vec4_test, clamp_inside_bounds_returns_same)
{
    vec4 val(5.0f, 7.0f, 9.0f, 11.0f);
    vec4 minv(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 maxv(10.0f, 20.0f, 30.0f, 40.0f);
    vec4 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 7.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
    EXPECT_FLOAT_EQ(result.w, 11.0f);
}

TEST_F(vec4_test, clamp_below_min_clamps_to_min)
{
    vec4 val(0.0f, 1.0f, 2.0f, 3.0f);
    vec4 minv(4.0f, 5.0f, 6.0f, 7.0f);
    vec4 maxv(10.0f, 20.0f, 30.0f, 40.0f);
    vec4 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
    EXPECT_FLOAT_EQ(result.w, 7.0f);
}

TEST_F(vec4_test, clamp_above_max_clamps_to_max)
{
    vec4 val(12.0f, 15.0f, 24.0f, 25.0f);
    vec4 minv(2.0f, 3.0f, 4.0f, 5.0f);
    vec4 maxv(10.0f, 15.0f, 20.0f, 25.0f);
    vec4 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 10.0f);
    EXPECT_FLOAT_EQ(result.y, 15.0f);
    EXPECT_FLOAT_EQ(result.z, 20.0f);
    EXPECT_FLOAT_EQ(result.w, 25.0f);
}

TEST_F(vec4_test, dot_returns_scalar_product)
{
    vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 70.0f);
}

TEST_F(vec4_test, dot_orthogonal_vectors_returns_zero)
{
    vec4 a(1.0f, 0.0f, 1.0f, 0.0f);
    vec4 b(0.0f, 1.0f, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 0.0f);
}

TEST_F(vec4_test, length_returns_correct_value)
{
    vec4 v(3.0f, 4.0f, 5.0f, 6.0f);
    EXPECT_FLOAT_EQ(length(v), 9.273618495495704f);
}

TEST_F(vec4_test, length_zero_vector_is_zero)
{
    vec4 v(0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(length(v), 0.0f);
}

TEST_F(vec4_test, lerp_zero_coef_returns_a)
{
    vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    vec4 result = lerp(a, b, 0.0f);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
    EXPECT_FLOAT_EQ(result.w, 4.0f);
}

TEST_F(vec4_test, lerp_one_coef_returns_b)
{
    vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    vec4 result = lerp(a, b, 1.0f);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FLOAT_EQ(result.z, 7.0f);
    EXPECT_FLOAT_EQ(result.w, 8.0f);
}

TEST_F(vec4_test, lerp_half_coef_interpolates)
{
    vec4 a(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 b(10.0f, 20.0f, 30.0f, 40.0f);
    vec4 result = lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FLOAT_EQ(result.z, 15.0f);
    EXPECT_FLOAT_EQ(result.w, 20.0f);
}

TEST_F(vec4_test, lerp_quater_coef_interpolates)
{
    vec4 a(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 b(10.0f, 20.0f, 30.0f, 40.0f);
    vec4 result = lerp(a, b, 0.25f);
    EXPECT_FLOAT_EQ(result.x, 2.5f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
    EXPECT_FLOAT_EQ(result.z, 7.5f);
    EXPECT_FLOAT_EQ(result.w, 10.0f);
}

TEST_F(vec4_test, lerp_minus_one_coef_extrapolations)
{
    vec4 a(5.0f, 10.0f, 15.0f, 20.0f);
    vec4 b(10.0f, 20.0f, 30.0f, 40.0f);
    vec4 result = lerp(a, b, -1.0f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
    EXPECT_FLOAT_EQ(result.w, 0.0f);
}

TEST_F(vec4_test, lerp_two_coef_extrapolations)
{
    vec4 a(5.0f, 10.0f, 15.0f, 20.0f);
    vec4 b(10.0f, 20.0f, 30.0f, 40.0f);
    vec4 result = lerp(a, b, 2.0f);
    EXPECT_FLOAT_EQ(result.x, 15.0f);
    EXPECT_FLOAT_EQ(result.y, 30.0f);
    EXPECT_FLOAT_EQ(result.z, 45.0f);
    EXPECT_FLOAT_EQ(result.w, 60.0f);
}

TEST_F(vec4_test, max_returns_componentwise_max)
{
    vec4 a(1.0f, 5.0f, 9.0f, 13.0f);
    vec4 b(3.0f, 2.0f, 4.0f, 6.0f);
    vec4 result = max(a, b);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
    EXPECT_FLOAT_EQ(result.w, 13.0f);
}

TEST_F(vec4_test, min_returns_componentwise_min)
{
    vec4 a(1.0f, 5.0f, 9.0f, 13.0f);
    vec4 b(3.0f, 2.0f, 4.0f, 6.0f);
    vec4 result = min(a, b);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
    EXPECT_FLOAT_EQ(result.w, 6.0f);
}
TEST_F(vec4_test, normalize_returns_unit_vector)
{
    vec4  v(3.0f, 4.0f, 5.0f, 6.0f);
    vec4  n = normalize(v);
    float len = length(n);
    EXPECT_NEAR(len, 1.0f, 1e-6f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec4_test, normalize_zero_vector_calls_assert)
{
    vec4 v(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 n = normalize(v);
    EXPECT_TRUE(assert_was_called());
}
