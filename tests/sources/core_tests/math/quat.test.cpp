#include <common.test.hpp>

#include <tavros/core/math.hpp>

using namespace tavros::math;

class quat_test : public unittest_scope
{
};

TEST_F(quat_test, default_constructor_produces_identity)
{
    quat q;
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

TEST_F(quat_test, from_axis_angle_constructs_correct_quaternion)
{
    vec3  axis = normalize(vec3{0.0f, 1.0f, 0.0f});
    float angle_rad = k_pi<float> / 2.0f; // 90 degrees

    quat q = quat::from_axis_angle(axis, angle_rad);

    // Expect 90 degree rotation around Y axis
    EXPECT_NEAR(q.x, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.y, std::sin(angle_rad / 2.0f), k_epsilon5);
    EXPECT_NEAR(q.z, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.w, std::cos(angle_rad / 2.0f), k_epsilon5);
}

TEST_F(quat_test, from_euler_constructs_correct_quaternion)
{
    // Rotate 90 degrees around X
    euler3 euler{k_pi<float> / 2.0f, 0.0f, 0.0f};

    quat q = quat::from_euler(euler);

    // Should be 90 degree rotation around X
    EXPECT_NEAR(q.x, std::sin(k_pi<float> / 4.0f), k_epsilon5);
    EXPECT_NEAR(q.y, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.z, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.w, std::cos(k_pi<float> / 4.0f), k_epsilon5);
}

TEST_F(quat_test, from_to_rotation_same_vector_returns_identity)
{
    vec3 v = normalize(vec3{1.0f, 0.0f, 0.0f});
    quat q = quat::from_to_rotation(v, v);

    // If from == to, should be identity quaternion
    EXPECT_NEAR(q.x, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.y, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.z, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.w, 1.0f, k_epsilon5);
}

TEST_F(quat_test, from_to_rotation_opposite_vectors_returns_180_degree_rotation)
{
    vec3 from = normalize(vec3{1.0f, 0.0f, 0.0f});
    vec3 to = normalize(vec3{-1.0f, 0.0f, 0.0f});

    quat q = quat::from_to_rotation(from, to);

    // Rotation around any axis perpendicular to from
    // Should be 180 degree rotation
    EXPECT_NEAR(q.w, 0.0f, k_epsilon5);                         // cos(180/2) = 0
    EXPECT_NEAR(length(vec3{q.x, q.y, q.z}), 1.0f, k_epsilon5); // axis should be normalized
}

TEST_F(quat_test, look_rotation_forward_returns_identity)
{
    vec3 forward = normalize(vec3{1.0f, 0.0f, 0.0f}); // X forward
    vec3 up = normalize(vec3{0.0f, 0.0f, 1.0f});      // Z up

    quat q = quat::look_rotation(forward, up);

    // If forward = X, up = Z, should be identity rotation
    EXPECT_NEAR(q.x, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.y, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.z, 0.0f, k_epsilon5);
    EXPECT_NEAR(q.w, 1.0f, k_epsilon5);
}

TEST_F(quat_test, look_rotation_rotates_forward_vector_correctly)
{
    vec3 forward = normalize(vec3{1.0f, 0.0f, 0.0f}); // X
    vec3 up = normalize(vec3{0.0f, 0.0f, 1.0f});      // Z

    quat q = quat::look_rotation(forward, up);

    // Check that forward after rotation is X
    vec3 rotated_forward = q * vec3{1.0f, 0.0f, 0.0f};
    EXPECT_NEAR(rotated_forward.x, 1.0f, k_epsilon5);
    EXPECT_NEAR(rotated_forward.y, 0.0f, k_epsilon5);
    EXPECT_NEAR(rotated_forward.z, 0.0f, k_epsilon5);

    // Check that up after rotation is Z
    vec3 rotated_up = q * vec3{0.0f, 0.0f, 1.0f};
    EXPECT_NEAR(rotated_up.x, 0.0f, k_epsilon5);
    EXPECT_NEAR(rotated_up.y, 0.0f, k_epsilon5);
    EXPECT_NEAR(rotated_up.z, 1.0f, k_epsilon5);
}

TEST_F(quat_test, identity_returns_identity_quaternion)
{
    quat q = quat::identity();
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

TEST_F(quat_test, component_constructor_sets_correct_values)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(q.x, 1.0f);
    EXPECT_FLOAT_EQ(q.y, 2.0f);
    EXPECT_FLOAT_EQ(q.z, 3.0f);
    EXPECT_FLOAT_EQ(q.w, 4.0f);
}

TEST_F(quat_test, operator_index_const_returns_correct_values)
{
    const quat q(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(q[0], 1.0f);
    EXPECT_FLOAT_EQ(q[1], 2.0f);
    EXPECT_FLOAT_EQ(q[2], 3.0f);
    EXPECT_FLOAT_EQ(q[3], 4.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(quat_test, operator_index_nonconst_modifies_components)
{
    quat q;
    q[0] = 5.0f;
    q[1] = 6.0f;
    q[2] = 7.0f;
    q[3] = 8.0f;
    EXPECT_FLOAT_EQ(q.x, 5.0f);
    EXPECT_FLOAT_EQ(q.y, 6.0f);
    EXPECT_FLOAT_EQ(q.z, 7.0f);
    EXPECT_FLOAT_EQ(q.w, 8.0f);
}

TEST_F(quat_test, operator_index_out_of_bounds_triggers_assert)
{
    const quat q;
    auto       a = q[0];
    auto       b = q[1];
    auto       c = q[2];
    auto       d = q[3];
    EXPECT_FALSE(assert_was_called());
    auto ub = q[4];
    EXPECT_TRUE(assert_was_called());
}

TEST_F(quat_test, operator_add_assign_works)
{
    quat q1(1.0f, 2.0f, 3.0f, 4.0f);
    quat q2(4.0f, 3.0f, 2.0f, 1.0f);
    q1 += q2;
    EXPECT_FLOAT_EQ(q1.x, 5.0f);
    EXPECT_FLOAT_EQ(q1.y, 5.0f);
    EXPECT_FLOAT_EQ(q1.z, 5.0f);
    EXPECT_FLOAT_EQ(q1.w, 5.0f);
}

TEST_F(quat_test, operator_sub_assign_works)
{
    quat q1(5.0f, 7.0f, 9.0f, 11.0f);
    quat q2(1.0f, 2.0f, 3.0f, 4.0f);
    q1 -= q2;
    EXPECT_FLOAT_EQ(q1.x, 4.0f);
    EXPECT_FLOAT_EQ(q1.y, 5.0f);
    EXPECT_FLOAT_EQ(q1.z, 6.0f);
    EXPECT_FLOAT_EQ(q1.w, 7.0f);
}

TEST_F(quat_test, operator_mul_assign_scalar_works)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    q *= 2.0f;
    EXPECT_FLOAT_EQ(q.x, 2.0f);
    EXPECT_FLOAT_EQ(q.y, 4.0f);
    EXPECT_FLOAT_EQ(q.z, 6.0f);
    EXPECT_FLOAT_EQ(q.w, 8.0f);
}

TEST_F(quat_test, operator_div_assign_scalar_works)
{
    quat q(2.0f, 4.0f, 6.0f, 8.0f);
    q /= 2.0f;
    EXPECT_FLOAT_EQ(q.x, 1.0f);
    EXPECT_FLOAT_EQ(q.y, 2.0f);
    EXPECT_FLOAT_EQ(q.z, 3.0f);
    EXPECT_FLOAT_EQ(q.w, 4.0f);
    EXPECT_FALSE(assert_was_called());
}

TEST_F(quat_test, operator_div_assign_scalar_zero_calls_assert)
{
    quat q(2.0f, 4.0f, 6.0f, 8.0f);
    q /= 0.0f;
    EXPECT_TRUE(assert_was_called());
}

TEST_F(quat_test, data_const_returns_pointer_to_components)
{
    const quat   q(1.0f, 2.0f, 3.0f, 4.0f);
    const float* d = q.data();
    EXPECT_FLOAT_EQ(d[0], 1.0f);
    EXPECT_FLOAT_EQ(d[1], 2.0f);
    EXPECT_FLOAT_EQ(d[2], 3.0f);
    EXPECT_FLOAT_EQ(d[3], 4.0f);
}

TEST_F(quat_test, data_nonconst_returns_pointer_to_components)
{
    quat   q(1.0f, 2.0f, 3.0f, 4.0f);
    float* d = q.data();
    d[0] = 5.0f;
    d[1] = 6.0f;
    d[2] = 7.0f;
    d[3] = 8.0f;
    EXPECT_FLOAT_EQ(q.x, 5.0f);
    EXPECT_FLOAT_EQ(q.y, 6.0f);
    EXPECT_FLOAT_EQ(q.z, 7.0f);
    EXPECT_FLOAT_EQ(q.w, 8.0f);
}

TEST_F(quat_test, rotate_identity)
{
    quat q = quat::identity();
    vec3 v{1.0f, 2.0f, 3.0f};

    vec3 result = q * v;

    EXPECT_NEAR(result.x, v.x, k_epsilon5);
    EXPECT_NEAR(result.y, v.y, k_epsilon5);
    EXPECT_NEAR(result.z, v.z, k_epsilon5);
}

TEST_F(quat_test, rotate_90_deg_around_z)
{
    quat q = quat::from_axis_angle(vec3{0, 0, 1}, deg_to_rad(90.0f));
    vec3 v{1.0f, 0.0f, 0.0f};

    vec3 result = q * v;

    EXPECT_NEAR(result.x, 0.0f, k_epsilon5);
    EXPECT_NEAR(result.y, -1.0f, k_epsilon5);
    EXPECT_NEAR(result.z, 0.0f, k_epsilon5);
}

TEST_F(quat_test, rotate_180_deg_around_y)
{
    quat q = quat::from_axis_angle(vec3{0, 1, 0}, deg_to_rad(180.0f));
    vec3 v{1.0f, 0.0f, 0.0f};

    vec3 result = q * v;

    EXPECT_NEAR(result.x, -1.0f, k_epsilon5);
    EXPECT_NEAR(result.y, 0.0f, k_epsilon5);
    EXPECT_NEAR(result.z, 0.0f, k_epsilon5);
}

TEST_F(quat_test, rotate_90_deg_around_x)
{
    quat q = quat::from_axis_angle(vec3(1.0f, 0.0f, 0.0f), deg_to_rad(90.0f));
    vec3 v{0.0f, 0.0f, 1.0f};

    vec3 result = q * v;

    EXPECT_NEAR(result.x, 0.0f, k_epsilon5);
    EXPECT_NEAR(result.y, 1.0f, k_epsilon5);
    EXPECT_NEAR(result.z, 0.0f, k_epsilon5);
}

TEST_F(quat_test, rotate_nontrivial_vector)
{
    quat q = quat::from_axis_angle(vec3(1.0f, 1.0f, 0.0f), deg_to_rad(90.0f));
    vec3 v{1.0f, 0.0f, 0.0f};

    vec3 result = q * v;

    // Expect to be in the direction of Y+ and Z- (a complex case)
    EXPECT_NEAR(result.x, 0.5f, k_epsilon5);
    EXPECT_NEAR(result.y, 0.5f, k_epsilon5);
    EXPECT_NEAR(result.z, 0.7071067f, k_epsilon5);
}

TEST_F(quat_test, negate_operator)
{
    quat q{1.0f, -2.0f, 3.0f, -4.0f};
    quat result = -q;
    EXPECT_FLOAT_EQ(result.x, -1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, -3.0f);
    EXPECT_FLOAT_EQ(result.w, 4.0f);
}

TEST_F(quat_test, addition_operator)
{
    quat a{1.0f, 2.0f, 3.0f, 4.0f};
    quat b{0.5f, -1.5f, 2.5f, -3.5f};
    quat result = a + b;
    EXPECT_FLOAT_EQ(result.x, 1.5f);
    EXPECT_FLOAT_EQ(result.y, 0.5f);
    EXPECT_FLOAT_EQ(result.z, 5.5f);
    EXPECT_FLOAT_EQ(result.w, 0.5f);
}

TEST_F(quat_test, subtraction_operator)
{
    quat a{1.0f, 2.0f, 3.0f, 4.0f};
    quat b{0.5f, 1.0f, 1.5f, 2.0f};
    quat result = a - b;
    EXPECT_FLOAT_EQ(result.x, 0.5f);
    EXPECT_FLOAT_EQ(result.y, 1.0f);
    EXPECT_FLOAT_EQ(result.z, 1.5f);
    EXPECT_FLOAT_EQ(result.w, 2.0f);
}

TEST_F(quat_test, quaternion_multiplication_operator)
{
    quat a = quat::from_axis_angle(vec3(1.0f, 0.0f, 0.0f), k_pi<float>); // 180 deg around X
    quat b = quat::from_axis_angle(vec3(0.0f, 1.0f, 0.0f), k_pi<float>); // 180 deg around Y
    quat result = a * b;
    EXPECT_NEAR(result.x, 0.0f, k_epsilon5);
    EXPECT_NEAR(result.y, 0.0f, k_epsilon5);
    EXPECT_NEAR(result.z, 1.0f, k_epsilon5);
    EXPECT_NEAR(result.w, 0.0f, k_epsilon5);
}

TEST_F(quat_test, quaternion_multiplication_operator2)
{
    quat a = quat::from_axis_angle(vec3(1.0f, 0.0f, 0.0f), k_pi<float> / 2.0f); // 90 deg around X
    quat b = quat::from_axis_angle(vec3(0.0f, 1.0f, 0.0f), k_pi<float> / 2.0f); // 90 deg around Y
    quat c = quat::from_axis_angle(vec3(0.0f, 0.0f, 1.0f), k_pi<float> / 2.0f); // 90 deg around z
    quat result = a * b * c;
    EXPECT_NEAR(result.x, 0.7071f, k_epsilon5);
    EXPECT_NEAR(result.y, 0.0f, k_epsilon5);
    EXPECT_NEAR(result.z, 0.7071f, k_epsilon5);
    EXPECT_NEAR(result.w, 0.0f, k_epsilon5);
}

TEST_F(quat_test, rotate_vector_operator)
{
    quat rotation{0.0f, 0.0f, std::sinf(k_pi<float> / 4.0f), std::cos(k_pi<float> / 4.0f)}; // 90 deg around Z
    vec3 point{1.0f, 0.0f, 0.0f};

    vec3 rotated = rotation * point;

    EXPECT_NEAR(rotated.x, 0.0f, k_epsilon5);
    EXPECT_NEAR(rotated.y, -1.0f, k_epsilon5);
    EXPECT_NEAR(rotated.z, 0.0f, k_epsilon5);
}

TEST_F(quat_test, scalar_multiplication_right)
{
    quat q{1.0f, -2.0f, 3.0f, -4.0f};
    quat result = q * 2.0f;
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, -4.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
    EXPECT_FLOAT_EQ(result.w, -8.0f);
}

TEST_F(quat_test, scalar_multiplication_left)
{
    quat q{1.0f, -2.0f, 3.0f, -4.0f};
    quat result = 2.0f * q;
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, -4.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
    EXPECT_FLOAT_EQ(result.w, -8.0f);
}

TEST_F(quat_test, scalar_division_right)
{
    quat q{2.0f, -4.0f, 6.0f, -8.0f};
    quat result = q / 2.0f;
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, -2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
    EXPECT_FLOAT_EQ(result.w, -4.0f);
}

TEST_F(quat_test, scalar_division_left)
{
    quat q{2.0f, 4.0f, 8.0f, 16.0f};
    quat result = 32.0f / q;
    EXPECT_FLOAT_EQ(result.x, 16.0f);
    EXPECT_FLOAT_EQ(result.y, 8.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
    EXPECT_FLOAT_EQ(result.w, 2.0f);
}
