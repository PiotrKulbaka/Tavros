#include <common.test.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>

using namespace tavros::math;
using namespace tavros::geometry;

class sphere_test : public unittest_scope
{
};

TEST_F(sphere_test, default_constructor)
{
    sphere s;
    EXPECT_EQ(s.center.x, 0.0f);
    EXPECT_EQ(s.center.y, 0.0f);
    EXPECT_EQ(s.center.z, 0.0f);
    EXPECT_EQ(s.radius, 0.0f);
}

TEST_F(sphere_test, parameterized_constructor)
{
    vec3  center(1.0f, 2.0f, 3.0f);
    float radius = 5.0f;

    sphere s(center, radius);
    EXPECT_EQ(s.center.x, 1.0f);
    EXPECT_EQ(s.center.y, 2.0f);
    EXPECT_EQ(s.center.z, 3.0f);
    EXPECT_EQ(s.radius, 5.0f);
}

TEST_F(sphere_test, project_point_inside_sphere)
{
    vec3   point(1.0f, 1.0f, 1.0f);
    sphere s(vec3(0.0f, 0.0f, 0.0f), 5.0f);

    auto result = project_point(point, s);

    // Since the point is inside the sphere, it will be projected onto the sphere surface
    EXPECT_FLOAT_EQ(result.x, 1.0f * 5.0f / tavros::math::sqrt(3.0f));
    EXPECT_FLOAT_EQ(result.y, 1.0f * 5.0f / tavros::math::sqrt(3.0f));
    EXPECT_FLOAT_EQ(result.z, 1.0f * 5.0f / tavros::math::sqrt(3.0f));
}

TEST_F(sphere_test, project_point_outside_sphere)
{
    vec3   point(10.0f, 0.0f, 0.0f);
    sphere s(vec3(0.0f, 0.0f, 0.0f), 5.0f);

    auto result = project_point(point, s);

    // The point is outside the sphere and should be projected onto the surface
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

TEST_F(sphere_test, project_point_in_center_sphere)
{
    vec3   point(5.0f, 10.0f, 15.0f);
    sphere s(vec3(5.0f, 10.0f, 15.0f), 5.0f);

    auto result = project_point(point, s);

    // The point is outside the sphere and should be projected onto the surface
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FLOAT_EQ(result.z, 20.0f);
}

TEST_F(sphere_test, intersect_no_intersection)
{
    ray3   r(vec3(10.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    sphere s(vec3(0.0f, 0.0f, 0.0f), 5.0f);

    float t = intersect(r, s);
    EXPECT_EQ(t, 0.0f);
}

TEST_F(sphere_test, intersect_ray_hits_sphere)
{
    ray3   r(vec3(10.0f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f));
    sphere s(vec3(0.0f, 0.0f, 0.0f), 5.0f);

    float t = intersect(r, s);
    EXPECT_GT(t, 0.0f);
}

TEST_F(sphere_test, intersect_ray_inside_sphere)
{
    ray3   r(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f));
    sphere s(vec3(0.0f, 0.0f, 0.0f), 5.0f);

    float t = intersect(r, s);
    EXPECT_GT(t, 0.0f);
}

TEST_F(sphere_test, distance_point_inside_sphere)
{
    sphere s(vec3(0.0f, 0.0f, 0.0f), 5.0f);
    vec3   point(1.0f, 1.0f, 1.0f);

    float dist = distance(s, point);
    EXPECT_LT(dist, 0.0f); // Point is inside the sphere
}

TEST_F(sphere_test, distance_point_on_surface)
{
    sphere s(vec3(0.0f, 0.0f, 0.0f), 5.0f);
    vec3   point(5.0f, 0.0f, 0.0f);

    float dist = distance(s, point);
    EXPECT_EQ(dist, 0.0f); // Point is on the surface
}

TEST_F(sphere_test, distance_point_outside_sphere)
{
    sphere s(vec3(0.0f, 0.0f, 0.0f), 5.0f);
    vec3   point(10.0f, 0.0f, 0.0f);

    float dist = distance(s, point);
    EXPECT_GT(dist, 0.0f); // Point is outside the sphere
}
