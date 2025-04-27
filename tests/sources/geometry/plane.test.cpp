#include <common.test.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>

using namespace tavros::math;
using namespace tavros::geometry;

class plane_test : public unittest_scope
{
};

TEST_F(plane_test, default_constructor)
{
    plane p;
    EXPECT_EQ(p.normal.x, 0.0f);
    EXPECT_EQ(p.normal.y, 0.0f);
    EXPECT_EQ(p.normal.z, 1.0f);
    EXPECT_EQ(p.normal.d, 1.0f);
}

TEST_F(plane_test, constructor_with_normal_and_d)
{
    vec3  n{1.0f, 2.0f, 3.0f};
    float d = -4.0f;
    plane p(n, d);

    EXPECT_EQ(p.normal.x, 1.0f);
    EXPECT_EQ(p.normal.y, 2.0f);
    EXPECT_EQ(p.normal.z, 3.0f);
    EXPECT_EQ(p.d, -4.0f);
}

TEST_F(plane_test, from_normal_point_basic)
{
    vec3 n{0.0f, 0.0f, 1.0f};
    vec3 point{1.0f, 2.0f, 5.0f};

    plane p = plane::from_normal_point(n, point);

    EXPECT_NEAR(dot(p.normal, point) + p.d, 0.0f, k_epsilon6);
    EXPECT_EQ(p.normal.x, 0.0f);
    EXPECT_EQ(p.normal.y, 0.0f);
    EXPECT_EQ(p.normal.z, 1.0f);
}

TEST_F(plane_test, from_points_basic)
{
    vec3 a(-40.0f, -10.0f, 0.0f);
    vec3 b(15.0f, 0.0f, 0.0f);
    vec3 c(0.0f, 45.0f, 0.0f);

    plane p = plane::from_points(a, b, c);

    EXPECT_NEAR(p.normal.x, 0.0f, k_epsilon6);
    EXPECT_NEAR(p.normal.y, 0.0f, k_epsilon6);
    EXPECT_NEAR(p.normal.z, -1.0f, k_epsilon6);

    EXPECT_NEAR(dot(p.normal, a) + p.d, 0.0f, k_epsilon6);
    EXPECT_NEAR(dot(p.normal, b) + p.d, 0.0f, k_epsilon6);
    EXPECT_NEAR(dot(p.normal, c) + p.d, 0.0f, k_epsilon6);
}

TEST_F(plane_test, from_points_basic_negative_dir)
{
    vec3 a(0.0f, 4.0f, 0.0f);
    vec3 b(8.0f, 0.0f, 0.0f);
    vec3 c(-3.0f, -11.0f, 0.0f);

    plane p = plane::from_points(a, b, c);

    EXPECT_NEAR(p.normal.x, 0.0f, k_epsilon6);
    EXPECT_NEAR(p.normal.y, 0.0f, k_epsilon6);
    EXPECT_NEAR(p.normal.z, 1.0f, k_epsilon6);

    EXPECT_NEAR(dot(p.normal, a) + p.d, 0.0f, k_epsilon6);
    EXPECT_NEAR(dot(p.normal, b) + p.d, 0.0f, k_epsilon6);
    EXPECT_NEAR(dot(p.normal, c) + p.d, 0.0f, k_epsilon6);
}

TEST_F(plane_test, from_points_tilted_plane)
{
    vec3 a(0.0f, 0.0f, 0.0f);
    vec3 b(1.0f, 0.0f, 1.0f);
    vec3 c(0.0f, 1.0f, 1.0f);

    plane p = plane::from_points(a, b, c);

    EXPECT_NEAR(dot(p.normal, a) + p.d, 0.0f, k_epsilon6);
    EXPECT_NEAR(dot(p.normal, b) + p.d, 0.0f, k_epsilon6);
    EXPECT_NEAR(dot(p.normal, c) + p.d, 0.0f, k_epsilon6);
}

TEST_F(plane_test, project_point_point_on_plane)
{
    plane p(vec3(0.0f, 0.0f, 1.0f), 0.0f);
    vec3  point{1.0f, 2.0f, 0.0f};

    vec3 projected = project_point(p, point);

    EXPECT_NEAR(projected.x, point.x, k_epsilon6);
    EXPECT_NEAR(projected.y, point.y, k_epsilon6);
    EXPECT_NEAR(projected.z, point.z, k_epsilon6);
}

TEST_F(plane_test, project_point_above_plane)
{
    plane p(vec3(0.0f, 0.0f, 1.0f), 0.0f);
    vec3  point(1.0f, 2.0f, 5.0f);

    vec3 projected = project_point(p, point);

    EXPECT_NEAR(dot(p.normal, projected) + p.d, 0.0f, k_epsilon6);
}

TEST_F(plane_test, project_point_below_plane)
{
    plane p(vec3(0.0f, 0.0f, 1.0f), 0.0f);
    vec3  point(1.0f, 2.0f, -5.0f);

    vec3 projected = project_point(p, point);

    EXPECT_NEAR(dot(p.normal, projected) + p.d, 0.0f, k_epsilon6);
}

TEST_F(plane_test, distance_point_on_plane)
{
    plane p(vec3(0.0f, 0.0f, 1.0f), 0.0f);
    vec3  point(1.0f, 2.0f, 0.0f);

    float dist = distance(p, point);

    EXPECT_NEAR(dist, 0.0f, k_epsilon6);
}

TEST_F(plane_test, distance_point_above_plane)
{
    plane p(vec3(0.0f, 0.0f, 1.0f), 0.0f);
    vec3  point(1.0f, 2.0f, 5.0f);

    float dist = distance(p, point);

    EXPECT_NEAR(dist, 5.0f, k_epsilon6);
}

TEST_F(plane_test, distance_point_below_plane)
{
    plane p(vec3(0.0f, 0.0f, 1.0f), 0.0f);
    vec3  point(1.0f, 2.0f, -5.0f);

    float dist = distance(p, point);

    EXPECT_NEAR(dist, -5.0f, k_epsilon6);
}
TEST_F(plane_test, tilted_plane_45_degrees_project_point)
{
    // Plane tilted 45deg to XY plane, normal along (0, 0, 1) + (1, 0, 0)
    vec3  normal = normalize(vec3(1.0f, 0.0f, 1.0f));
    vec3  point_on_plane = vec3(0.0f, 0.0f, 0.0f);
    plane p = plane::from_normal_point(normal, point_on_plane);

    vec3 point(1.0f, 0.0f, 0.0f); // above plane
    vec3 projected = project_point(p, point);

    EXPECT_NEAR(dot(p.normal, projected) + p.d, 0.0f, k_epsilon5);
}

TEST_F(plane_test, tilted_plane_random_normal_project_point)
{
    vec3  normal = normalize(vec3(1.0f, 2.0f, 3.0f));
    vec3  point_on_plane = vec3(0.0f, 0.0f, 0.0f);
    plane p = plane::from_normal_point(normal, point_on_plane);

    vec3 point(5.0f, -3.0f, 2.0f);
    vec3 projected = project_point(p, point);

    EXPECT_NEAR(dot(p.normal, projected) + p.d, 0.0f, k_epsilon5);
}

TEST_F(plane_test, tilted_plane_90_degrees_project_point)
{
    // Vertical plane, normal along X
    vec3  normal = vec3(1.0f, 0.0f, 0.0f);
    vec3  point_on_plane = vec3(2.0f, 0.0f, 0.0f);
    plane p = plane::from_normal_point(normal, point_on_plane);

    vec3 point(5.0f, 1.0f, 1.0f);
    vec3 projected = project_point(p, point);

    EXPECT_NEAR(dot(p.normal, projected) + p.d, 0.0f, k_epsilon5);
}

TEST_F(plane_test, tilted_plane_45_degrees_distance_positive)
{
    vec3  normal = normalize(vec3(1.0f, 0.0f, 1.0f));
    vec3  point_on_plane = vec3(0.0f, 0.0f, 0.0f);
    plane p = plane::from_normal_point(normal, point_on_plane);

    vec3  point(1.0f, 0.0f, 1.0f);
    float dist = distance(p, point);

    // Point above plane
    EXPECT_GT(dist, 0.0f);
}

TEST_F(plane_test, tilted_plane_45_degrees_distance_negative)
{
    vec3  normal = normalize(vec3(1.0f, 0.0f, 1.0f));
    vec3  point_on_plane = vec3(0.0f, 0.0f, 0.0f);
    plane p = plane::from_normal_point(normal, point_on_plane);

    vec3  point(-1.0f, 0.0f, -1.0f);
    float dist = distance(p, point);

    // Point below plane
    EXPECT_LT(dist, 0.0f);
}

TEST_F(plane_test, from_points_tilted_exact)
{
    vec3 a(0.0f, 0.0f, 0.0f);
    vec3 b(1.0f, 0.0f, 1.0f);
    vec3 c(0.0f, 1.0f, 1.0f);

    plane p = plane::from_points(a, b, c);

    EXPECT_NEAR(length(p.normal), 1.0f, k_epsilon5);

    // Check that all points lie on the plane
    EXPECT_NEAR(dot(p.normal, a) + p.d, 0.0f, k_epsilon5);
    EXPECT_NEAR(dot(p.normal, b) + p.d, 0.0f, k_epsilon5);
    EXPECT_NEAR(dot(p.normal, c) + p.d, 0.0f, k_epsilon5);
}
