#include <common.test.hpp>

#include <tavros/core/math.hpp>

#define M_PI       3.14159265358979323846   // pi
#define M_PI_2     1.57079632679489661923   // pi/2
#define M_PI_4     0.785398163397448309616  // pi/4
#define M_1_PI     0.318309886183790671538  // 1/pi
#define M_2_PI     0.636619772367581343076  // 2/pi

using namespace tavros::math;

class vec3_test : public unittest_scope
{
};

TEST_F(vec3_test, default_constructor_sets_zero)
{
    vec3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST_F(vec3_test, scalar_constructor_sets_both_components)
{
    vec3 v(3.14f);
    EXPECT_FLOAT_EQ(v.x, 3.14f);
    EXPECT_FLOAT_EQ(v.y, 3.14f);
    EXPECT_FLOAT_EQ(v.z, 3.14f);
}

TEST_F(vec3_test, component_constructor_sets_correct_values)
{
    vec3 v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST_F(vec3_test, operator_index_const_returns_correct_values)
{
    const vec3 v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[1], 2.0f);
    EXPECT_FLOAT_EQ(v[2], 3.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec3_test, operator_index_assertion_bounds)
{
    const vec3 v;
    float      pad;
    auto       x = v[0];
    auto       y = v[1];
    auto       z = v[2];
    EXPECT_FALSE(assert_was_called());
    auto ub = v[3]; // Out of bounds
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, operator_index_nonconst_modifies_components)
{
    vec3 v;
    v[0] = 5.0f;
    v[1] = 10.0f;
    v[2] = 15.0f;
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 10.0f);
    EXPECT_FLOAT_EQ(v.z, 15.0f);
}

TEST_F(vec3_test, operator_add_assign_works)
{
    vec3 v1(1.0f, 2.0f, 3.0f);
    vec3 v2(4.0f, 5.0f, 6.0f);
    v1 += v2;
    EXPECT_FLOAT_EQ(v1.x, 5.0f);
    EXPECT_FLOAT_EQ(v1.y, 7.0f);
    EXPECT_FLOAT_EQ(v1.z, 9.0f);
}

TEST_F(vec3_test, operator_sub_assign_works)
{
    vec3 v1(5.0f, 7.0f, 9.0f);
    vec3 v2(2.0f, 3.0f, 4.0f);
    v1 -= v2;
    EXPECT_FLOAT_EQ(v1.x, 3.0f);
    EXPECT_FLOAT_EQ(v1.y, 4.0f);
    EXPECT_FLOAT_EQ(v1.z, 5.0f);
}

TEST_F(vec3_test, operator_mul_scalar_works)
{
    vec3 v(2.0f, 3.0f, 4.0f);
    v *= 2.0f;
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 6.0f);
    EXPECT_FLOAT_EQ(v.z, 8.0f);
}

TEST_F(vec3_test, operator_div_scalar_works)
{
    vec3 v(4.0f, 8.0f, 12.0f);
    v /= 2.0f;
    EXPECT_FLOAT_EQ(v.x, 2.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
    EXPECT_FLOAT_EQ(v.z, 6.0f);
}

TEST_F(vec3_test, operator_div_scalar_zero)
{
    vec3 v(4.0f, 8.0f, 12.0f);
    v /= 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, operator_div_vector_componentwise_works)
{
    vec3 v(8.0f, 16.0f, 24.0f);
    vec3 divisor(2.0f, 4.0f, 6.0f);
    v /= divisor;
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
    EXPECT_FLOAT_EQ(v.z, 4.0f);
}

TEST_F(vec3_test, operator_div_zero_vector_componentwise)
{
    vec3 v(8.0f, 16.0f, 24.0f);
    vec3 divisor(0.0f, 0.0f, 0.0f);
    v /= divisor;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, data_returns_pointer_to_components)
{
    vec3   v(1.0f, 2.0f, 3.0f);
    float* data = v.data();
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 2.0f);
    EXPECT_FLOAT_EQ(data[2], 3.0f);
}

TEST_F(vec3_test, data_const_returns_pointer_to_components)
{
    const vec3   v(1.0f, 2.0f, 3.0f);
    const float* data = v.data();
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 2.0f);
    EXPECT_FLOAT_EQ(data[2], 3.0f);
}

TEST_F(vec3_test, unary_minus_operator_negates_components)
{
    vec3 v(3.0f, -4.0f, 5.0f);
    vec3 result = -v;
    EXPECT_FLOAT_EQ(result.x, -3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, -5.0f);
}

TEST_F(vec3_test, binary_add_operator_adds_components)
{
    vec3 a(1.0f, 2.0f, 3.0f);
    vec3 b(4.0f, 5.0f, 6.0f);
    vec3 result = a + b;
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 7.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
}

TEST_F(vec3_test, binary_sub_operator_subtracts_components)
{
    vec3 a(5.0f, 6.0f, 7.0f);
    vec3 b(2.0f, 3.0f, 4.0f);
    vec3 result = a - b;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(vec3_test, componentwise_multiplication_works)
{
    vec3 a(2.0f, 3.0f, 4.0f);
    vec3 b(4.0f, 5.0f, 6.0f);
    vec3 result = a * b;
    EXPECT_FLOAT_EQ(result.x, 8.0f);
    EXPECT_FLOAT_EQ(result.y, 15.0f);
    EXPECT_FLOAT_EQ(result.z, 24.0f);
}

TEST_F(vec3_test, scalar_right_multiplication_works)
{
    vec3 v(2.0f, 3.0f, 4.0f);
    vec3 result = v * 2.0f;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FLOAT_EQ(result.z, 8.0f);
}

TEST_F(vec3_test, scalar_left_multiplication_works)
{
    vec3 v(2.0f, 3.0f, 4.0f);
    vec3 result = 2.0f * v;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FLOAT_EQ(result.z, 8.0f);
}

TEST_F(vec3_test, componentwise_division_works)
{
    vec3 a(8.0f, 18.0f, 24.0f);
    vec3 b(2.0f, 3.0f, 4.0f);
    vec3 result = a / b;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec3_test, componentwise_division_by_zero_calls_assert)
{
    vec3 a(8.0f, 18.0f, 27.0f);
    vec3 b(0.0f, 1.0f, 2.0f);
    vec3 result = a / b;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, scalar_division_works)
{
    vec3 v(10.0f, 20.0f, 30.0f);
    vec3 result = v / 2.0f;
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FLOAT_EQ(result.z, 15.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec3_test, scalar_division_by_zero_calls_assert)
{
    vec3 v(10.0f, 20.0f, 30.0f);
    vec3 result = v / 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, scalar_divided_by_vector_works)
{
    vec3 v(2.0f, 4.0f, 6.0f);
    vec3 result = 12.0f / v;
    EXPECT_FLOAT_EQ(result.x, 6.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 2.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec3_test, scalar_divided_by_zero_vector_calls_assert)
{
    vec3 v(0.0f, 1.0f, 2.0f);
    vec3 result = 12.0f / v;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, almost_equal_true_for_close_vectors)
{
    vec3 a(1.00000105f, 2.00000105f, 3.00000105f);
    vec3 b(1.000002f, 2.000002f, 3.000002f);
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(vec3_test, almost_equal_false_for_different_vectors)
{
    vec3 a(1.0f, 2.0f, 3.0f);
    vec3 b(1.0f, 3.0f, 3.0f);
    EXPECT_FALSE(almost_equal(a, b));
}

TEST_F(vec3_test, clamp_inside_bounds_returns_same)
{
    vec3 val(5.0f, 7.0f, 9.0f);
    vec3 minv(1.0f, 2.0f, 3.0f);
    vec3 maxv(10.0f, 20.0f, 30.0f);
    vec3 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 7.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
}

TEST_F(vec3_test, clamp_below_min_clamps_to_min)
{
    vec3 val(0.0f, 1.0f, 2.0f);
    vec3 minv(2.0f, 2.0f, 2.0f);
    vec3 maxv(10.0f, 10.0f, 10.0f);
    vec3 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 2.0f);
}

TEST_F(vec3_test, clamp_above_max_clamps_to_max)
{
    vec3 val(12.0f, 15.0f, 17.0f);
    vec3 minv(2.0f, 2.0f, 2.0f);
    vec3 maxv(10.0f, 10.0f, 10.0f);
    vec3 result = clamp(val, minv, maxv);
    EXPECT_FLOAT_EQ(result.x, 10.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FLOAT_EQ(result.z, 10.0f);
}

TEST_F(vec3_test, cross_perpendicular_vectors_returns_non_zero)
{
    vec3 a(1.0f, 0.0f, 0.0f);
    vec3 b(0.0f, 1.0f, 0.0f);
    vec3 result = cross(a, b);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 1.0f);
}

TEST_F(vec3_test, cross_reverse_perpendicular_vectors_returns_non_zero)
{
    vec3 a(0.0f, 1.0f, 0.0f);
    vec3 b(1.0f, 0.0f, 0.0f);
    vec3 result = cross(a, b);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, -1.0f);
}

TEST_F(vec3_test, cross_parallel_vectors_returns_zero)
{
    vec3 a(1.0f, 2.0f, 3.0f);
    vec3 b(2.0f, 4.0f, 6.0f);
    vec3 result = cross(a, b);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

TEST_F(vec3_test, cross_with_zero_vector_returns_zero)
{
    vec3 a(1.0f, 0.0f, 0.0f);
    vec3 b(0.0f, 0.0f, 0.0f);
    vec3 result = cross(a, b);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

TEST_F(vec3_test, cross_perpendicular_vectors_x_axis)
{
    vec3 a(0.0f, 1.0f, 0.0f);
    vec3 b(0.0f, 0.0f, 1.0f);
    vec3 result = cross(a, b);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

TEST_F(vec3_test, cross_perpendicular_vectors_y_axis)
{
    vec3 a(1.0f, 0.0f, 0.0f);
    vec3 b(0.0f, 0.0f, 1.0f);
    vec3 result = cross(a, b);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, -1.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

TEST_F(vec3_test, dot_returns_scalar_product)
{
    vec3 a(1.0f, 2.0f, 3.0f);
    vec3 b(4.0f, 5.0f, 6.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 32.0f);
}

TEST_F(vec3_test, dot_orthogonal_vectors_returns_zero)
{
    vec3 a(1.0f, 0.0f, 0.0f);
    vec3 b(0.0f, 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 0.0f);
}

TEST_F(vec3_test, length_returns_correct_value)
{
    vec3 v(3.0f, 4.0f, 12.0f);
    EXPECT_FLOAT_EQ(length(v), 13.0f);
}

TEST_F(vec3_test, length_zero_vector_is_zero)
{
    vec3 v(0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(length(v), 0.0f);
}

TEST_F(vec3_test, lerp_zero_coef_returns_a)
{
    vec3 a(1.0f, 2.0f, 3.0f);
    vec3 b(5.0f, 6.0f, 7.0f);
    vec3 result = lerp(a, b, 0.0f);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(vec3_test, lerp_one_coef_returns_b)
{
    vec3 a(1.0f, 2.0f, 3.0f);
    vec3 b(5.0f, 6.0f, 7.0f);
    vec3 result = lerp(a, b, 1.0f);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
    EXPECT_FLOAT_EQ(result.z, 7.0f);
}

TEST_F(vec3_test, lerp_half_coef_interpolates)
{
    vec3 a(0.0f, 0.0f, 0.0f);
    vec3 b(10.0f, 20.0f, 30.0f);
    vec3 result = lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FLOAT_EQ(result.z, 15.0f);
}

TEST_F(vec3_test, lerp_quater_coef_interpolates)
{
    vec3 a(0.0f, 0.0f, 0.0f);
    vec3 b(10.0f, 20.0f, 30.0f);
    vec3 result = lerp(a, b, 0.25f);
    EXPECT_FLOAT_EQ(result.x, 2.5f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
    EXPECT_FLOAT_EQ(result.z, 7.5f);
}

TEST_F(vec3_test, lerp_two_coef_extrapolates)
{
    vec3 a(0.0f, 0.0f, 0.0f);
    vec3 b(10.0f, 20.0f, 30.0f);
    vec3 result = lerp(a, b, 2.0f);
    EXPECT_FLOAT_EQ(result.x, 20.0f);
    EXPECT_FLOAT_EQ(result.y, 40.0f);
    EXPECT_FLOAT_EQ(result.z, 60.0f);
}

TEST_F(vec3_test, lerp_minus_one_coef_extrapolates)
{
    vec3 a(0.0f, 0.0f, 0.0f);
    vec3 b(10.0f, 20.0f, 30.0f);
    vec3 result = lerp(a, b, -1.0f);
    EXPECT_FLOAT_EQ(result.x, -10.0f);
    EXPECT_FLOAT_EQ(result.y, -20.0f);
    EXPECT_FLOAT_EQ(result.z, -30.0f);
}

TEST_F(vec3_test, max_returns_componentwise_max)
{
    vec3 a(1.0f, 5.0f, 9.0f);
    vec3 b(3.0f, 2.0f, 7.0f);
    vec3 result = max(a, b);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
}

TEST_F(vec3_test, min_returns_componentwise_min)
{
    vec3 a(1.0f, 5.0f, 9.0f);
    vec3 b(3.0f, 2.0f, 7.0f);
    vec3 result = min(a, b);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 7.0f);
}

TEST_F(vec3_test, normalize_returns_unit_vector)
{
    vec3  v(3.0f, 4.0f, 12.0f);
    vec3  n = normalize(v);
    float len = length(n);
    EXPECT_NEAR(len, 1.0f, 1e-6f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(vec3_test, normalize_zero_vector_calls_assert)
{
    vec3 v(0.0f, 0.0f, 0.0f);
    vec3 n = normalize(v);
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, angle_between_identical_vectors_is_zero)
{
    vec3  a(1.0f, 0.0f, 0.0f);
    vec3  b(1.0f, 0.0f, 0.0f);
    float result = angle_between(a, b);
    EXPECT_FLOAT_EQ(result, 0.0f);
}

TEST_F(vec3_test, angle_between_perpendicular_vectors_is_90_degrees)
{
    vec3  a(1.0f, 0.0f, 0.0f);
    vec3  b(0.0f, 1.0f, 0.0f);
    float result = angle_between(a, b);
    EXPECT_FLOAT_EQ(result, M_PI_2); // 90 degrees in radians
}

TEST_F(vec3_test, angle_between_opposite_vectors_is_180_degrees)
{
    vec3  a(1.0f, 0.0f, 0.0f);
    vec3  b(-1.0f, 0.0f, 0.0f);
    float result = angle_between(a, b);
    EXPECT_FLOAT_EQ(result, M_PI); // 180 degrees in radians
}

TEST_F(vec3_test, angle_between_non_perpendicular_vectors_is_45_degrees)
{
    vec3  a(1.0f, 1.0f, 0.0f);
    vec3  b(1.0f, 0.0f, 0.0f);
    float result = angle_between(a, b);
    EXPECT_FLOAT_EQ(result, M_PI_4); // 45 degrees in radians
}

TEST_F(vec3_test, angle_between_vectors_with_acute_angle)
{
    vec3  a(1.0f, 2.0f, 0.0f);
    vec3  b(2.0f, 1.0f, 0.0f);
    float result = angle_between(a, b);
    float result_deg = rad_to_deg(result);
    EXPECT_NEAR(result_deg, 36.86989764584401f, k_epsilon6);
}

TEST_F(vec3_test, angle_between_vectors_with_obtuse_angle)
{
    vec3  a(1.0f, 0.0f, 0.0f);
    vec3  b(-1.0f, 0.0f, 0.0f);
    float result = angle_between(a, b);
    EXPECT_FLOAT_EQ(result, M_PI); // 180 degrees in radians
}

TEST_F(vec3_test, angle_between_zero_vector_should_return_zero)
{
    vec3  a(0.0f, 0.0f, 0.0f);
    vec3  b(1.0f, 0.0f, 0.0f);
    float result = angle_between(a, b);
    EXPECT_FLOAT_EQ(result, 0.0f);
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, angle_between_zero_vector_should_return_zero_2)
{
    vec3  a(0.0f, 0.0f, 1.0f);
    vec3  b(0.0f, 0.0f, 0.0f);
    float result = angle_between(a, b);
    EXPECT_FLOAT_EQ(result, 0.0f);
    EXPECT_TRUE(assert_was_called());
}

TEST_F(vec3_test, ortohonal_vector)
{
    vec3  v(2.0f, 12.0f, 34.0f);
    vec3  orto = orthogonal(v);
    float result = angle_between(v, orto);
    auto  deg = rad_to_deg(result);
    EXPECT_NEAR(deg, 90.0f, k_epsilon6);
}

TEST_F(vec3_test, ortohonal_vector_2)
{
    vec3  v(-33.0f, 66.0f, -3.0f);
    vec3  orto = orthogonal(v);
    float result = angle_between(v, orto);
    auto  deg = rad_to_deg(result);
    EXPECT_NEAR(deg, 90.0f, k_epsilon6);
}

TEST_F(vec3_test, ortohonal_vector_3)
{
    vec3  v(-0.00001f, 0.0001f, 0.0000005f);
    vec3  orto = orthogonal(v);
    float result = angle_between(v, orto);
    auto  deg = rad_to_deg(result);
    EXPECT_NEAR(deg, 90.0f, k_epsilon6);
}
