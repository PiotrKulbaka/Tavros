#include <common.test.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>

using namespace tavros::math;
using namespace tavros::geometry;

class ray3_test : public unittest_scope
{
};

TEST_F(ray3_test, default_constructor)
{
    ray3 r;
    EXPECT_EQ(r.origin.x, 0.0f);
    EXPECT_EQ(r.origin.y, 0.0f);
    EXPECT_EQ(r.origin.z, 0.0f);

    EXPECT_EQ(r.direction.x, 0.0f);
    EXPECT_EQ(r.direction.y, 0.0f);
    EXPECT_EQ(r.direction.z, 1.0f);
}


TEST_F(ray3_test, at_returns_correct_point)
{
    ray3 r(vec3(1.0f, 2.0f, 3.0f), vec3(1.0f, 0.0f, 0.0f));
    vec3 point = r.at(5.0f);

    EXPECT_NEAR(point.x, 6.0f, k_epsilon6);
    EXPECT_NEAR(point.y, 2.0f, k_epsilon6);
    EXPECT_NEAR(point.z, 3.0f, k_epsilon6);
}

TEST_F(ray3_test, at_zero_returns_origin)
{
    ray3 r(vec3(1.0f, 2.0f, 3.0f), vec3(0.0f, 1.0f, 0.0f));
    vec3 point = r.at(0.0f);

    EXPECT_NEAR(point.x, 1.0f, k_epsilon6);
    EXPECT_NEAR(point.y, 2.0f, k_epsilon6);
    EXPECT_NEAR(point.z, 3.0f, k_epsilon6);
}

TEST_F(ray3_test, at_negative_t_works)
{
    ray3 r(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    vec3 point = r.at(-2.0f);

    EXPECT_NEAR(point.x, 0.0f, k_epsilon6);
    EXPECT_NEAR(point.y, -2.0f, k_epsilon6);
    EXPECT_NEAR(point.z, 0.0f, k_epsilon6);
}

TEST_F(ray3_test, point_on_plane_remains_same)
{
    plane plane{vec3{0.0f, 1.0f, 0.0f}, -5.0f};
    vec3  point{2.0f, 5.0f, -3.0f}; // Lies on the plane

    vec3 projected = project_point(plane, point);

    EXPECT_FLOAT_EQ(projected.x, point.x);
    EXPECT_FLOAT_EQ(projected.y, point.y);
    EXPECT_FLOAT_EQ(projected.z, point.z);
}

TEST_F(ray3_test, point_above_plane_projects_down)
{
    plane plane{vec3{0.0f, 1.0f, 0.0f}, 0.0f};
    vec3  point{1.0f, 5.0f, 1.0f}; // Above the plane y=0

    vec3 projected = project_point(plane, point);

    EXPECT_FLOAT_EQ(projected.x, 1.0f);
    EXPECT_FLOAT_EQ(projected.y, 0.0f);
    EXPECT_FLOAT_EQ(projected.z, 1.0f);
}

TEST_F(ray3_test, point_below_plane_projects_up)
{
    plane plane{vec3{0.0f, 0.0f, 1.0f}, -2.0f};
    vec3  point{0.0f, 0.0f, 0.0f}; // Below the plane z=2

    vec3 projected = project_point(plane, point);

    EXPECT_FLOAT_EQ(projected.x, 0.0f);
    EXPECT_FLOAT_EQ(projected.y, 0.0f);
    EXPECT_FLOAT_EQ(projected.z, 2.0f);
}

TEST_F(ray3_test, project_on_tilted_plane)
{
    // Plane tilted 45 degrees relative to the Z axis
    plane plane{normalize(vec3{0.0f, 0.0f, 1.0f}), -2.0f};
    vec3  point{3.0f, 4.0f, 10.0f}; // Above the plane

    vec3 projected = project_point(plane, point);

    // Expect z == 2.0f after projection
    EXPECT_NEAR(projected.z, 2.0f, 1e-4f);
}

TEST_F(ray3_test, project_on_diagonal_plane)
{
    // Plane: normal is the diagonal XYZ
    plane plane{normalize(vec3{1.0f, 1.0f, 1.0f}), -5.0f};
    vec3  point{2.0f, 2.0f, 2.0f};

    vec3 projected = project_point(plane, point);

    // Check that the point is on the plane after projection
    float dist = dot(plane.normal, projected) + plane.d;
    EXPECT_NEAR(dist, 0.0f, 1e-4f);
}

TEST_F(ray3_test, ray_hits_plane_straight)
{
    ray3  ray{vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    plane plane{vec3{0.0f, 1.0f, 0.0f}, -5.0f};

    float t = intersect(ray, plane);

    EXPECT_GT(t, 0.0f);

    vec3 hit_point = ray.at(t);
    EXPECT_NEAR(hit_point.y, 5.0f, 1e-4f);
}

TEST_F(ray3_test, ray_parallel_to_plane_no_hit)
{
    ray3  ray{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}};
    plane plane{vec3{0.0f, 1.0f, 0.0f}, -5.0f};

    float t = intersect(ray, plane);

    EXPECT_FLOAT_EQ(t, 0.0f);
}

TEST_F(ray3_test, ray_behind_plane_no_hit)
{
    ray3  ray{vec3{0.0f, 10.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}}; // Ray goes up, plane is below
    plane plane{vec3{0.0f, 1.0f, 0.0f}, -5.0f};

    float t = intersect(ray, plane);

    EXPECT_FLOAT_EQ(t, 0.0f);
}

TEST_F(ray3_test, ray_starts_on_plane_but_goes_away)
{
    ray3  ray{vec3{0.0f, 5.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}}; // Ray starts at the plane and goes away
    plane plane{vec3{0.0f, 1.0f, 0.0f}, -5.0f};

    float t = intersect(ray, plane);

    EXPECT_FLOAT_EQ(t, 0.0f);
}

TEST_F(ray3_test, ray_hits_tilted_plane)
{
    ray3  ray{vec3{0.0f, 0.0f, 0.0f}, normalize(vec3{0.0f, 1.0f, 1.0f})};
    plane plane{normalize(vec3{0.0f, 0.0f, 1.0f}), -5.0f}; // Plane parallel to XY

    float t = intersect(ray, plane);

    EXPECT_GT(t, 0.0f);

    vec3 hit_point = ray.at(t);
    EXPECT_NEAR(hit_point.z, 5.0f, 1e-4f);
}

TEST_F(ray3_test, ray_misses_due_to_direction)
{
    ray3  ray{vec3{0.0f, 0.0f, 0.0f}, normalize(vec3{0.0f, -1.0f, -1.0f})};
    plane plane{normalize(vec3{0.0f, 0.0f, 1.0f}), -5.0f};

    float t = intersect(ray, plane);

    EXPECT_FLOAT_EQ(t, 0.0f);
}

TEST_F(ray3_test, ray_almost_parallel_small_angle_hit)
{
    ray3  ray{vec3{0.0f, 0.0f, 0.0f}, normalize(vec3{0.0f, 1.0f, 0.01f})};
    plane plane{normalize(vec3{0.0f, 0.0f, 1.0f}), -5.0f};

    float t = intersect(ray, plane);

    EXPECT_GT(t, 0.0f);

    vec3 hit_point = ray.at(t);
    EXPECT_NEAR(hit_point.z, 5.0f, 1e-3f); // Larger tolerance due to very small angle
}
