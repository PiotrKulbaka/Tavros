#include "render_app_base.hpp"

#include "free_camera.hpp"

#include <tavros/core/type_info.hpp>

#include <tavros/assets/asset_manager.hpp>
#include <tavros/assets/providers/filesystem_provider.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/math.hpp>
#include <tavros/renderer/render_system.hpp>
#include <tavros/renderer/renderer/debug_renderer.hpp>
#include <tavros/system/application.hpp>
#include <tavros/tef/loader.hpp>
#include <tavros/input/input_manager.hpp>

#include <tavros/renderer/text/text_builder.hpp>
#include <tavros/renderer/text/text_layouter.hpp>
#include <tavros/renderer/renderer/ui_renderer.hpp>

#include <tavros/ui/root_view.hpp>
#include <tavros/ui/linear_view.hpp>
#include <tavros/ui/static/label.hpp>

#include <tavros/core/debug/debug.hpp>

#include <tracy/Tracy.hpp>

constexpr uint32 k_msaa = 1;

namespace fs = tavros::filesystem;
namespace rhi = tavros::renderer::rhi;

static tavros::core::logger logger("main");


namespace tavros::renderer::dbg
{
    using vec3 = math::vec3;
    using col = math::color32;

    constexpr float k_pi = 3.14159265358979323846f;
    constexpr float k_two_pi = 2.0f * k_pi;

    inline float fl(uint32 v) noexcept
    {
        return static_cast<float>(v);
    }

    inline vec3 unit_x() noexcept
    {
        return vec3(1.0f, 0.0f, 0.0f);
    }
    inline vec3 unit_y() noexcept
    {
        return vec3(0.0f, 1.0f, 0.0f);
    }
    inline vec3 unit_z() noexcept
    {
        return vec3(0.0f, 0.0f, 1.0f);
    }

    // ----------------------------------------------------------------- colors

    inline col rgba(float r, float g, float b, float a = 1.0f) noexcept
    {
        auto u8 = [](float x) { return static_cast<uint8>(std::clamp(x, 0.0f, 1.0f) * 255.0f + 0.5f); };
        return col(u8(r), u8(g), u8(b), u8(a));
    }

    // h in [0, 1) wraps around
    inline col hsv(float h, float s = 1.0f, float v = 1.0f, float a = 1.0f) noexcept
    {
        h -= std::floor(h);
        const float k = h * 6.0f;
        const int   i = static_cast<int>(k);
        const float f = k - static_cast<float>(i);
        const float p = v * (1.0f - s);
        const float q = v * (1.0f - s * f);
        const float t = v * (1.0f - s * (1.0f - f));
        switch (i % 6) {
        case 0:
            return rgba(v, t, p, a);
        case 1:
            return rgba(q, v, p, a);
        case 2:
            return rgba(p, v, t, a);
        case 3:
            return rgba(p, q, v, a);
        case 4:
            return rgba(t, p, v, a);
        default:
            return rgba(v, p, q, a);
        }
    }

    // Deterministic hash -> [0, 1)
    inline float hash01(uint32 x) noexcept
    {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return static_cast<float>(x & 0xFFFFFFu) / 16777216.0f;
    }

    // ------------------------------------------------------------------- math

    inline vec3 cross3(vec3 a, vec3 b) noexcept
    {
        return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }
    inline float dot3(vec3 a, vec3 b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline vec3 norm3(vec3 a) noexcept
    {
        const float l = std::sqrt(dot3(a, a));
        return l > 1e-8f ? a * (1.0f / l) : unit_z();
    }

    // u x v == n, so u*cos(a) + v*sin(a) goes counter-clockwise seen from +n
    inline void basis_from_normal(vec3 n, vec3& u, vec3& v) noexcept
    {
        n = norm3(n);
        const vec3 h = std::abs(n.z) < 0.99f ? unit_z() : unit_x();
        u = norm3(cross3(h, n));
        v = cross3(n, u);
    }

    inline vec3 face_normal(vec3 a, vec3 b, vec3 c) noexcept
    {
        return norm3(cross3(b - a, c - a));
    }

    // Horizontal unit vector at angle a (counter-clockwise from +X)
    inline vec3 radial_xy(float a) noexcept
    {
        return vec3(std::cos(a), std::sin(a), 0.0f);
    }

    // Circle in the XY plane, counter-clockwise seen from +Z
    inline vec3 ring_point(vec3 c, float r, float a) noexcept
    {
        return vec3(c.x + r * std::cos(a), c.y + r * std::sin(a), c.z);
    }

    // theta - polar angle from +Z, phi - azimuth in XY from +X towards +Y
    inline vec3 sphere_point(vec3 c, float r, float theta, float phi) noexcept
    {
        return vec3(
            c.x + r * std::sin(theta) * std::cos(phi),
            c.y + r * std::sin(theta) * std::sin(phi),
            c.z + r * std::cos(theta)
        );
    }

    // Outward normal of a cone side (apex along +Z) at angle a
    inline vec3 cone_normal(float a, float h, float r) noexcept
    {
        return norm3(radial_xy(a) * h + unit_z() * r);
    }

    // ------------------------------------------------------------- primitives

    // Flat-shaded triangles: the normal comes from the winding
    inline void tri(debug_renderer& d, vec3 a, vec3 b, vec3 c, col cl)
    {
        const vec3 n = face_normal(a, b, c);
        d.triangle(a, n, b, n, c, n, cl);
    }

    inline void tri(debug_renderer& d, vec3 a, col ca, vec3 b, col cb, vec3 c, col cc)
    {
        const vec3 n = face_normal(a, b, c);
        d.triangle(a, n, ca, b, n, cb, c, n, cc);
    }

    // Polyline with a per-vertex color. pos(t) -> vec3, color(t) -> col, t in [0, 1]
    template<typename Pos, typename Color>
    void curve(debug_renderer& d, uint32 segments, Pos&& pos, Color&& color)
    {
        vec3  prev = pos(0.0f);
        float prev_t = 0.0f;
        for (uint32 i = 1; i <= segments; ++i) {
            const float t = fl(i) / fl(segments);
            const vec3  cur = pos(t);
            d.line(prev, color(prev_t), cur, color(t));
            prev = cur;
            prev_t = t;
        }
    }

    // The same with a normal: nrm(t) -> vec3
    template<typename Pos, typename Normal, typename Color>
    void curve_n(debug_renderer& d, uint32 segments, Pos&& pos, Normal&& nrm, Color&& color)
    {
        vec3  prev = pos(0.0f);
        vec3  prev_n = nrm(0.0f);
        float prev_t = 0.0f;
        for (uint32 i = 1; i <= segments; ++i) {
            const float t = fl(i) / fl(segments);
            const vec3  cur = pos(t);
            const vec3  cur_n = nrm(t);
            d.line(prev, prev_n, color(prev_t), cur, cur_n, color(t));
            prev = cur;
            prev_n = cur_n;
            prev_t = t;
        }
    }

    inline void circle(debug_renderer& d, vec3 c, vec3 u, vec3 v, float r, uint32 seg, col cl)
    {
        curve(
            d, seg,
            [&](float t) {
                const float a = t * k_two_pi;
                return c + (u * std::cos(a) + v * std::sin(a)) * r;
            },
            [&](float) { return cl; }
        );
    }

    inline void circle_n(debug_renderer& d, vec3 c, vec3 normal, float r, uint32 seg, col cl)
    {
        vec3 u, v;
        basis_from_normal(normal, u, v);
        circle(d, c, u, v, r, seg, cl);
    }

    // Filled disc, faces u x v
    inline void disc(debug_renderer& d, vec3 c, vec3 u, vec3 v, float r, uint32 seg, col center_cl, col edge_cl)
    {
        const vec3 n = norm3(cross3(u, v));
        for (uint32 i = 0; i < seg; ++i) {
            const float a0 = k_two_pi * fl(i) / fl(seg);
            const float a1 = k_two_pi * fl(i + 1) / fl(seg);
            const vec3  p0 = c + (u * std::cos(a0) + v * std::sin(a0)) * r;
            const vec3  p1 = c + (u * std::cos(a1) + v * std::sin(a1)) * r;
            d.triangle(c, n, center_cl, p0, n, edge_cl, p1, n, edge_cl);
        }
    }

    // Vertices go counter-clockwise (a, b, c, e); flat normal
    inline void quad_fill(debug_renderer& d, vec3 a, vec3 b, vec3 c, vec3 e, col cl)
    {
        tri(d, a, b, c, cl);
        tri(d, a, c, e, cl);
    }

    inline void quad_fill(debug_renderer& d, vec3 a, col ca, vec3 b, col cb, vec3 c, col cc, vec3 e, col ce)
    {
        tri(d, a, ca, b, cb, c, cc);
        tri(d, a, ca, c, cc, e, ce);
    }

    // Arrow: a line (no normal) with a smooth-shaded cone head at `to`
    inline void arrow(debug_renderer& d, vec3 from, vec3 to, col cl, float head_len = 0.25f, float head_r = 0.08f)
    {
        const vec3 dir = norm3(to - from);
        const vec3 base = to - dir * head_len;
        d.line(from, base, cl);

        vec3 u, v;
        basis_from_normal(dir, u, v);

        const float inv_slope = 1.0f / std::sqrt(head_len * head_len + head_r * head_r);
        auto        side_n = [&](float a) { return (u * std::cos(a) + v * std::sin(a)) * (head_len * inv_slope) + dir * (head_r * inv_slope); };

        constexpr uint32 seg = 64;
        for (uint32 i = 0; i < seg; ++i) {
            const float a0 = k_two_pi * fl(i) / fl(seg);
            const float a1 = k_two_pi * fl(i + 1) / fl(seg);
            const vec3  p0 = base + (u * std::cos(a0) + v * std::sin(a0)) * head_r;
            const vec3  p1 = base + (u * std::cos(a1) + v * std::sin(a1)) * head_r;
            const vec3  n0 = side_n(a0), n1 = side_n(a1);
            d.triangle(p0, n0, p1, n1, to, norm3(n0 + n1), cl);
            d.triangle(base, -dir, p1, -dir, p0, -dir, cl);
        }
    }

    // ---------------------------------------------------------------- helpers

    inline void axes(debug_renderer& d, vec3 o, float size)
    {
        arrow(d, o, o + vec3(size, 0.0f, 0.0f), rgba(1.0f, 0.2f, 0.2f), size * 0.2f, size * 0.06f);
        arrow(d, o, o + vec3(0.0f, size, 0.0f), rgba(0.2f, 1.0f, 0.2f), size * 0.2f, size * 0.06f);
        arrow(d, o, o + vec3(0.0f, 0.0f, size), rgba(0.3f, 0.5f, 1.0f), size * 0.2f, size * 0.06f);
    }

    // Grid on the XY plane (the floor). The lines through the center get the axis colors:
    // axis_x - the line along X, axis_y - the line along Y.
    inline void grid_xy(debug_renderer& d, vec3 c, float half, float cell, uint32 major_every, col minor, col major, col axis_x, col axis_y)
    {
        const int   n = static_cast<int>(half / cell);
        const float ext = static_cast<float>(n) * cell;
        for (int i = -n; i <= n; ++i) {
            const float o = static_cast<float>(i) * cell;
            const bool  is_major = major_every != 0 && (static_cast<uint32>(std::abs(i)) % major_every) == 0;
            const col   cl = is_major ? major : minor;
            d.line(vec3(c.x + o, c.y - ext, c.z), vec3(c.x + o, c.y + ext, c.z), i == 0 ? axis_y : cl); // along Y
            d.line(vec3(c.x - ext, c.y + o, c.z), vec3(c.x + ext, c.y + o, c.z), i == 0 ? axis_x : cl); // along X
        }
    }

    // ------------------------------------------------------------------- box

    // Corner bits: 1 - +x, 2 - +y, 4 - +z
    inline void box_corners(vec3 (&p)[8], vec3 c, vec3 h) noexcept
    {
        for (int i = 0; i < 8; ++i) {
            p[i] = vec3(
                c.x + ((i & 1) ? h.x : -h.x),
                c.y + ((i & 2) ? h.y : -h.y),
                c.z + ((i & 4) ? h.z : -h.z)
            );
        }
    }

    inline void wire_box(debug_renderer& d, vec3 c, vec3 h, col cl)
    {
        vec3 p[8];
        box_corners(p, c, h);
        for (int i = 0; i < 8; ++i) {
            for (int b = 0; b < 3; ++b) {
                const int j = i ^ (1 << b);
                if (i < j) {
                    d.line(p[i], p[j], cl);
                }
            }
        }
    }

    // Face order: +X, -X, +Y, -Y, +Z, -Z
    inline void solid_box(debug_renderer& d, vec3 c, vec3 h, const col (&faces)[6])
    {
        static constexpr int k_faces[6][4] = {{1, 3, 7, 5}, {0, 4, 6, 2}, {2, 6, 7, 3}, {0, 1, 5, 4}, {4, 5, 7, 6}, {0, 2, 3, 1}};
        vec3                 p[8];
        box_corners(p, c, h);
        for (int f = 0; f < 6; ++f) {
            quad_fill(d, p[k_faces[f][0]], p[k_faces[f][1]], p[k_faces[f][2]], p[k_faces[f][3]], faces[f]);
        }
    }

    inline void rainbow_faces(col (&out)[6], float alpha = 1.0f, float sat = 0.7f)
    {
        for (int i = 0; i < 6; ++i) {
            out[i] = hsv(static_cast<float>(i) / 6.0f, sat, 1.0f, alpha);
        }
    }

    // ---------------------------------------------------------------- sphere

    // Wires carry the radial normal, so the lines are shaded like a volume
    inline void wire_sphere(debug_renderer& d, vec3 c, float r, uint32 rings, uint32 meridians, col cl)
    {
        for (uint32 i = 1; i < rings; ++i) { // latitude circles
            const float theta = k_pi * fl(i) / fl(rings);
            const vec3  center(c.x, c.y, c.z + r * std::cos(theta));
            const float rr = r * std::sin(theta);
            auto        pos = [&](float t) { return ring_point(center, rr, t * k_two_pi); };
            curve_n(d, 32, pos, [&](float t) { return norm3(pos(t) - c); }, [&](float) { return cl; });
        }
        for (uint32 m = 0; m < meridians; ++m) {
            const float phi = k_two_pi * fl(m) / fl(meridians);
            auto        pos = [&](float t) { return sphere_point(c, r, k_pi * t, phi); };
            curve_n(d, 24, pos, [&](float t) { return norm3(pos(t) - c); }, [&](float) { return cl; });
        }
    }

    // color_at(theta, phi) -> col. Smooth shading: the normal is the radial direction
    template<typename ColorFn>
    void solid_sphere(debug_renderer& d, vec3 c, float r, uint32 rings, uint32 segs, ColorFn&& color_at)
    {
        auto theta_of = [&](uint32 i) { return k_pi * fl(i) / fl(rings); };
        auto phi_of = [&](uint32 j) { return k_two_pi * fl(j) / fl(segs); };

        auto P = [&](uint32 i, uint32 j) { return sphere_point(c, r, theta_of(i), phi_of(j)); };
        auto N = [&](uint32 i, uint32 j) { return sphere_point(vec3(0.0f, 0.0f, 0.0f), 1.0f, theta_of(i), phi_of(j)); };
        auto C = [&](uint32 i, uint32 j) { return color_at(theta_of(i), phi_of(j)); };

        for (uint32 i = 0; i < rings; ++i) {
            for (uint32 j = 0; j < segs; ++j) {
                d.triangle(
                    P(i, j), N(i, j), C(i, j),
                    P(i + 1, j), N(i + 1, j), C(i + 1, j),
                    P(i, j + 1), N(i, j + 1), C(i, j + 1)
                );
                d.triangle(
                    P(i, j + 1), N(i, j + 1), C(i, j + 1),
                    P(i + 1, j), N(i + 1, j), C(i + 1, j),
                    P(i + 1, j + 1), N(i + 1, j + 1), C(i + 1, j + 1)
                );
            }
        }
    }

    inline void solid_sphere(debug_renderer& d, vec3 c, float r, uint32 rings, uint32 segs)
    {
        solid_sphere(d, c, r, rings, segs, [](float theta, float phi) {
            return hsv(phi / k_two_pi, 0.75f, 1.0f - 0.45f * theta / k_pi);
        });
    }

    // -------------------------------------------------------- cylinder / cone
    // Both stand on `base` and grow along +Z

    inline void wire_cylinder(debug_renderer& d, vec3 base, float h, float r, uint32 seg, col cl)
    {
        const vec3 top(base.x, base.y, base.z + h);
        const vec3 centers[2] = {base, top};
        for (const vec3& center : centers) {
            curve_n(
                d, seg,
                [&](float t) { return ring_point(center, r, t * k_two_pi); },
                [&](float t) { return radial_xy(t * k_two_pi); },
                [&](float) { return cl; }
            );
        }
        for (uint32 i = 0; i < 8; ++i) {
            const float a = k_two_pi * fl(i) / 8.0f;
            const vec3  n = radial_xy(a);
            d.line(ring_point(base, r, a), n, ring_point(top, r, a), n, cl);
        }
    }

    inline void solid_cylinder(debug_renderer& d, vec3 base, float h, float r, uint32 seg, float hue = 0.0f)
    {
        const vec3 top(base.x, base.y, base.z + h);
        for (uint32 j = 0; j < seg; ++j) {
            const float a0 = k_two_pi * fl(j) / fl(seg);
            const float a1 = k_two_pi * fl(j + 1) / fl(seg);
            const vec3  b0 = ring_point(base, r, a0), b1 = ring_point(base, r, a1);
            const vec3  t0 = ring_point(top, r, a0), t1 = ring_point(top, r, a1);
            const vec3  n0 = radial_xy(a0), n1 = radial_xy(a1);
            const col   c0 = hsv(hue + a0 / k_two_pi, 0.7f), c1 = hsv(hue + a1 / k_two_pi, 0.7f);
            d.triangle(b0, n0, c0, b1, n1, c1, t0, n0, c0);
            d.triangle(b1, n1, c1, t1, n1, c1, t0, n0, c0);
        }
        disc(d, top, unit_x(), unit_y(), r, seg, rgba(0.95f, 0.95f, 0.95f), rgba(0.55f, 0.55f, 0.6f));  // faces +Z
        disc(d, base, unit_y(), unit_x(), r, seg, rgba(0.95f, 0.95f, 0.95f), rgba(0.55f, 0.55f, 0.6f)); // faces -Z
    }

    inline void wire_cone(debug_renderer& d, vec3 base, float h, float r, uint32 seg, col cl)
    {
        const vec3 apex(base.x, base.y, base.z + h);
        curve_n(
            d, seg,
            [&](float t) { return ring_point(base, r, t * k_two_pi); },
            [&](float t) { return cone_normal(t * k_two_pi, h, r); },
            [&](float) { return cl; }
        );
        for (uint32 i = 0; i < 8; ++i) {
            const float a = k_two_pi * fl(i) / 8.0f;
            const vec3  n = cone_normal(a, h, r);
            d.line(ring_point(base, r, a), n, apex, n, cl);
        }
    }

    inline void solid_cone(debug_renderer& d, vec3 base, float h, float r, uint32 seg, float hue = 0.0f)
    {
        const vec3 apex(base.x, base.y, base.z + h);
        for (uint32 j = 0; j < seg; ++j) {
            const float a0 = k_two_pi * fl(j) / fl(seg);
            const float a1 = k_two_pi * fl(j + 1) / fl(seg);
            const vec3  n0 = cone_normal(a0, h, r), n1 = cone_normal(a1, h, r);
            const col   c0 = hsv(hue + a0 / k_two_pi, 0.7f), c1 = hsv(hue + a1 / k_two_pi, 0.7f);
            d.triangle(ring_point(base, r, a0), n0, c0, ring_point(base, r, a1), n1, c1, apex, norm3(n0 + n1), rgba(1.0f, 1.0f, 1.0f));
        }
        disc(d, base, unit_y(), unit_x(), r, seg, rgba(0.9f, 0.9f, 0.9f), rgba(0.5f, 0.5f, 0.55f)); // faces -Z
    }

    // Cone from an apex along dir (spot light, view cone): a gizmo, no normals
    inline void wire_cone_dir(debug_renderer& d, vec3 apex, vec3 dir, float length, float half_angle, uint32 seg, col cl)
    {
        const vec3 n = norm3(dir);
        vec3       u, v;
        basis_from_normal(n, u, v);
        const vec3  bc = apex + n * length;
        const float r = std::tan(half_angle) * length;
        circle(d, bc, u, v, r, seg, cl);
        for (uint32 i = 0; i < 4; ++i) {
            const float a = k_two_pi * fl(i) / 4.0f;
            d.line(apex, bc + (u * std::cos(a) + v * std::sin(a)) * r, cl);
        }
        d.line(apex, bc, cl);
    }

    // ------------------------------------------------------------------ torus
    // Lies on the floor: u goes around the Z axis, v goes around the tube

    inline vec3 torus_point(vec3 c, float R, float r, float u, float v) noexcept
    {
        const float ring = R + r * std::cos(v);
        return vec3(c.x + ring * std::cos(u), c.y + ring * std::sin(u), c.z + r * std::sin(v));
    }

    // Unit direction from the tube center to the surface point
    inline vec3 torus_normal(float u, float v) noexcept
    {
        return vec3(std::cos(v) * std::cos(u), std::cos(v) * std::sin(u), std::sin(v));
    }

    inline void wire_torus(debug_renderer& d, vec3 c, float R, float r, uint32 rings, uint32 sides, col cl)
    {
        for (uint32 i = 0; i < rings; ++i) {
            const float u = k_two_pi * fl(i) / fl(rings);
            curve_n(
                d, 16,
                [&](float t) { return torus_point(c, R, r, u, t * k_two_pi); },
                [&](float t) { return torus_normal(u, t * k_two_pi); },
                [&](float) { return cl; }
            );
        }
        for (uint32 j = 0; j < sides; ++j) {
            const float v = k_two_pi * fl(j) / fl(sides);
            curve_n(
                d, 48,
                [&](float t) { return torus_point(c, R, r, t * k_two_pi, v); },
                [&](float t) { return torus_normal(t * k_two_pi, v); },
                [&](float) { return cl; }
            );
        }
    }

    inline void solid_torus(debug_renderer& d, vec3 c, float R, float r, uint32 rings, uint32 sides)
    {
        auto col_at = [](float u, float v) { return hsv(u / k_two_pi, 0.8f, 0.6f + 0.4f * (0.5f + 0.5f * std::sin(v))); };

        for (uint32 i = 0; i < rings; ++i) {
            for (uint32 j = 0; j < sides; ++j) {
                const float u0 = k_two_pi * fl(i) / fl(rings), u1 = k_two_pi * fl(i + 1) / fl(rings);
                const float v0 = k_two_pi * fl(j) / fl(sides), v1 = k_two_pi * fl(j + 1) / fl(sides);

                const vec3 A = torus_point(c, R, r, u0, v0), B = torus_point(c, R, r, u0, v1);
                const vec3 C = torus_point(c, R, r, u1, v1), D = torus_point(c, R, r, u1, v0);
                const vec3 nA = torus_normal(u0, v0), nB = torus_normal(u0, v1);
                const vec3 nC = torus_normal(u1, v1), nD = torus_normal(u1, v0);

                d.triangle(A, nA, col_at(u0, v0), D, nD, col_at(u1, v0), B, nB, col_at(u0, v1));
                d.triangle(B, nB, col_at(u0, v1), D, nD, col_at(u1, v0), C, nC, col_at(u1, v1));
            }
        }
    }

    // ------------------------------------------------------ platonic solids
    // Flat shading, defined by coordinates only (do not depend on which axis is "up")

    inline void tetra_vertices(vec3 (&v)[4], vec3 c, float r) noexcept
    {
        v[0] = c + norm3(vec3(1.0f, 1.0f, 1.0f)) * r;
        v[1] = c + norm3(vec3(1.0f, -1.0f, -1.0f)) * r;
        v[2] = c + norm3(vec3(-1.0f, 1.0f, -1.0f)) * r;
        v[3] = c + norm3(vec3(-1.0f, -1.0f, 1.0f)) * r;
    }

    inline void solid_tetra(debug_renderer& d, vec3 c, float r, float alpha = 1.0f)
    {
        static constexpr int k_f[4][3] = {{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}};
        vec3                 v[4];
        tetra_vertices(v, c, r);
        for (int f = 0; f < 4; ++f) {
            tri(d, v[k_f[f][0]], v[k_f[f][1]], v[k_f[f][2]], hsv(0.08f + 0.25f * static_cast<float>(f), 0.7f, 1.0f, alpha));
        }
    }

    inline void wire_tetra(debug_renderer& d, vec3 c, float r, col cl)
    {
        vec3 v[4];
        tetra_vertices(v, c, r);
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                d.line(v[i], v[j], cl);
            }
        }
    }

    inline void solid_octa(debug_renderer& d, vec3 c, float r, float alpha = 1.0f)
    {
        int idx = 0;
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sy = -1; sy <= 1; sy += 2) {
                for (int sz = -1; sz <= 1; sz += 2) {
                    const vec3 x = c + vec3(static_cast<float>(sx) * r, 0.0f, 0.0f);
                    const vec3 y = c + vec3(0.0f, static_cast<float>(sy) * r, 0.0f);
                    const vec3 z = c + vec3(0.0f, 0.0f, static_cast<float>(sz) * r);
                    const col  cl = hsv(static_cast<float>(idx++) / 8.0f, 0.65f, 1.0f, alpha);
                    // keep the winding outward: odd parity flips the order
                    if (sx * sy * sz > 0) {
                        tri(d, x, y, z, cl);
                    } else {
                        tri(d, x, z, y, cl);
                    }
                }
            }
        }
    }

    inline void wire_octa(debug_renderer& d, vec3 c, float r, col cl)
    {
        const vec3 p[6] = {
            c + vec3(r, 0.0f, 0.0f), c + vec3(-r, 0.0f, 0.0f),
            c + vec3(0.0f, r, 0.0f), c + vec3(0.0f, -r, 0.0f),
            c + vec3(0.0f, 0.0f, r), c + vec3(0.0f, 0.0f, -r)
        };
        for (int i = 0; i < 6; ++i) {
            for (int j = i + 1; j < 6; ++j) {
                if ((i ^ j) != 1) { // skip opposite vertices
                    d.line(p[i], p[j], cl);
                }
            }
        }
    }

    inline constexpr int k_icosa_faces[20][3] = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8}, {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    inline void icosa_vertices(vec3 (&v)[12], vec3 c, float r) noexcept
    {
        const float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;
        const vec3  raw[12] = {
            vec3(-1.0f, phi, 0.0f), vec3(1.0f, phi, 0.0f), vec3(-1.0f, -phi, 0.0f), vec3(1.0f, -phi, 0.0f),
            vec3(0.0f, -1.0f, phi), vec3(0.0f, 1.0f, phi), vec3(0.0f, -1.0f, -phi), vec3(0.0f, 1.0f, -phi),
            vec3(phi, 0.0f, -1.0f), vec3(phi, 0.0f, 1.0f), vec3(-phi, 0.0f, -1.0f), vec3(-phi, 0.0f, 1.0f)
        };
        for (int i = 0; i < 12; ++i) {
            v[i] = c + norm3(raw[i]) * r;
        }
    }

    inline void solid_icosa(debug_renderer& d, vec3 c, float r, float alpha = 1.0f)
    {
        vec3 v[12];
        icosa_vertices(v, c, r);
        for (int f = 0; f < 20; ++f) {
            tri(d, v[k_icosa_faces[f][0]], v[k_icosa_faces[f][1]], v[k_icosa_faces[f][2]], hsv(static_cast<float>(f) / 20.0f, 0.6f, 1.0f, alpha));
        }
    }

    inline void wire_icosa(debug_renderer& d, vec3 c, float r, col cl)
    {
        vec3 v[12];
        icosa_vertices(v, c, r);
        for (int f = 0; f < 20; ++f) {
            for (int e = 0; e < 3; ++e) {
                const int a = k_icosa_faces[f][e];
                const int b = k_icosa_faces[f][(e + 1) % 3];
                if (a < b) { // every shared edge is drawn once
                    d.line(v[a], v[b], cl);
                }
            }
        }
    }

    // ----------------------------------------------------- curves and others
    // Pure curves, gizmos and skeletons have no surface, so they are drawn without normals

    // Cubic bezier, hue gradient from hue0 to hue1
    inline void bezier3(debug_renderer& d, vec3 p0, vec3 p1, vec3 p2, vec3 p3, uint32 seg, float hue0, float hue1)
    {
        curve(
            d, seg,
            [&](float t) {
                const float u = 1.0f - t;
                return p0 * (u * u * u) + p1 * (3.0f * u * u * t) + p2 * (3.0f * u * t * t) + p3 * (t * t * t);
            },
            [&](float t) { return hsv(hue0 + (hue1 - hue0) * t); }
        );
    }

    // (p, q) torus knot, winds around the Z axis
    inline void torus_knot(debug_renderer& d, vec3 c, float scale, int p, int q, uint32 seg)
    {
        curve(
            d, seg,
            [&](float t) {
                const float a = t * k_two_pi;
                const float r = std::cos(static_cast<float>(q) * a) + 2.0f;
                return c + vec3(r * std::cos(static_cast<float>(p) * a), r * std::sin(static_cast<float>(p) * a), -std::sin(static_cast<float>(q) * a)) * scale;
            },
            [](float t) { return hsv(t); }
        );
    }

    // Double helix along +Z with rungs
    inline void dna(debug_renderer& d, vec3 base, float height, float radius, float turns, uint32 steps)
    {
        auto strand = [&](float t, float phase) {
            const float a = turns * k_two_pi * t + phase;
            return vec3(base.x + radius * std::cos(a), base.y + radius * std::sin(a), base.z + height * t);
        };

        curve(d, steps, [&](float t) { return strand(t, 0.0f); }, [](float) { return hsv(0.55f); });
        curve(d, steps, [&](float t) { return strand(t, k_pi); }, [](float) { return hsv(0.95f); });

        d.set_point_size(5.0f);
        for (uint32 i = 0; i <= steps; i += 3) {
            const float t = fl(i) / fl(steps);
            const vec3  a = strand(t, 0.0f), b = strand(t, k_pi);
            const col   rung = hsv((i / 3) % 2 == 0 ? 0.15f : 0.38f, 0.8f);
            d.line(a, rung, b, rung);
            d.point(a, hsv(0.55f, 0.5f));
            d.point(b, hsv(0.95f, 0.5f));
        }
        d.set_point_size(1.0f);
    }

    inline void frustum(debug_renderer& d, vec3 pos, vec3 forward, vec3 up_hint, float fov_y, float aspect, float znear, float zfar, col edge, col side)
    {
        const vec3  f = norm3(forward);
        const vec3  r = norm3(cross3(f, up_hint));
        const vec3  u = cross3(r, f);
        const float hn = std::tan(fov_y * 0.5f) * znear, wn = hn * aspect;
        const float hf = std::tan(fov_y * 0.5f) * zfar, wf = hf * aspect;
        const vec3  nc = pos + f * znear, fc = pos + f * zfar;

        const vec3 n[4] = {nc - r * wn - u * hn, nc + r * wn - u * hn, nc + r * wn + u * hn, nc - r * wn + u * hn};
        const vec3 fa[4] = {fc - r * wf - u * hf, fc + r * wf - u * hf, fc + r * wf + u * hf, fc - r * wf + u * hf};

        for (int i = 0; i < 4; ++i) {
            d.line(n[i], n[(i + 1) % 4], edge);
            d.line(fa[i], fa[(i + 1) % 4], edge);
            d.line(n[i], fa[i], edge);
            d.line(pos, n[i], side);
        }
    }

    // Stick figure standing on `root`, facing +Y: joints are points, bones are lines
    inline void skeleton(debug_renderer& d, vec3 root, float scale)
    {
        enum
        {
            pelvis,
            spine,
            chest,
            neck,
            head,
            sh_l,
            el_l,
            ha_l,
            sh_r,
            el_r,
            ha_r,
            hip_l,
            kn_l,
            ft_l,
            hip_r,
            kn_r,
            ft_r,
            count
        };

        // x - sideways, h - height (Z), f - forward (Y)
        auto J = [&](float x, float h, float f) { return root + vec3(x, f, h) * scale; };

        const vec3 j[count] = {
            J(0.0f, 1.0f, 0.0f), J(0.0f, 1.3f, 0.0f), J(0.0f, 1.6f, 0.0f), J(0.0f, 1.8f, 0.0f), J(0.0f, 2.0f, 0.0f),
            J(-0.25f, 1.7f, 0.0f), J(-0.5f, 1.45f, 0.05f), J(-0.6f, 1.2f, 0.15f),
            J(0.25f, 1.7f, 0.0f), J(0.5f, 1.45f, 0.05f), J(0.6f, 1.2f, 0.15f),
            J(-0.12f, 1.0f, 0.0f), J(-0.14f, 0.55f, 0.05f), J(-0.14f, 0.05f, 0.1f),
            J(0.12f, 1.0f, 0.0f), J(0.14f, 0.55f, 0.05f), J(0.14f, 0.05f, 0.1f)
        };

        // {from, to, group}: 0 - center, 1 - left, 2 - right
        const int bones[][3] = {
            {pelvis, spine, 0}, {spine, chest, 0}, {chest, neck, 0}, {neck, head, 0}, {chest, sh_l, 1}, {sh_l, el_l, 1}, {el_l, ha_l, 1}, {pelvis, hip_l, 1}, {hip_l, kn_l, 1}, {kn_l, ft_l, 1}, {chest, sh_r, 2}, {sh_r, el_r, 2}, {el_r, ha_r, 2}, {pelvis, hip_r, 2}, {hip_r, kn_r, 2}, {kn_r, ft_r, 2}
        };

        const col group_col[3] = {rgba(0.9f, 0.9f, 0.9f), hsv(0.5f, 0.8f), hsv(0.07f, 0.9f)};
        for (const auto& b : bones) {
            d.line(j[b[0]], j[b[1]], group_col[b[2]]);
        }

        d.set_point_size(7.0f);
        for (int i = 0; i < count; ++i) {
            d.point(j[i], rgba(1.0f, 1.0f, 0.4f));
        }
        d.set_point_size(1.0f);

        // The head: a circle in the XZ plane (faces forward)
        circle_n(d, j[head] + vec3(0.0f, 0.0f, 0.12f * scale), unit_y(), 0.12f * scale, 20, rgba(0.9f, 0.9f, 0.9f));
    }

    // ------------------------------------------------------------------- all

    // column -> X, row -> Y, z - height above the floor
    inline vec3 slot(int column, int row, float z = 1.5f) noexcept
    {
        return vec3((static_cast<float>(column) - 2.5f) * 4.5f, static_cast<float>(row) * 5.0f, z);
    }

    inline void reset_state(debug_renderer& d)
    {
        d.set_depth_test(debug_depth_test_mode::depth_test);
        d.set_point_size(1.0f);
    }

    // ----------------------------------------------------------- reference

    inline void showcase_reference(debug_renderer& d)
    {
        reset_state(d);
        // Centered at the origin: the colored lines are the real world X and Y axes
        grid_xy(d, vec3(0.0f, 0.0f, 0.0f), 24.0f, 1.0f, 5, rgba(0.25f, 0.25f, 0.28f), rgba(0.45f, 0.45f, 0.5f), rgba(1.0f, 0.25f, 0.25f), rgba(0.2f, 1.0f, 0.3f));
        axes(d, vec3(0.0f, 0.0f, 0.0f), 2.0f);
    }

    // ------------------------------------------------------------- wireframes

    inline void showcase_wireframes(debug_renderer& d)
    {
        reset_state(d);
        wire_box(d, slot(0, 0), vec3(1.0f, 1.0f, 1.0f), hsv(0.58f));
        wire_sphere(d, slot(1, 0), 1.3f, 8, 12, hsv(0.33f));
        wire_cylinder(d, slot(2, 0, 0.2f), 2.6f, 1.0f, 32, hsv(0.12f));
        wire_cone(d, slot(3, 0, 0.2f), 2.6f, 1.2f, 32, hsv(0.0f));
        wire_torus(d, slot(4, 0), 1.0f, 0.4f, 24, 12, hsv(0.8f));
        wire_icosa(d, slot(5, 0), 1.3f, hsv(0.5f));
    }

    // ------------------------------------------------------------------ solids

    inline void showcase_solids(debug_renderer& d)
    {
        reset_state(d);
        d.set_depth_test(debug_depth_test_mode::depth_write); // solids write depth and hide what is behind

        col faces[6];
        rainbow_faces(faces);
        solid_box(d, slot(0, 1), vec3(1.0f, 1.0f, 1.0f), faces);
        solid_sphere(d, slot(1, 1), 1.3f, 16, 24);
        solid_cylinder(d, slot(2, 1, 0.2f), 2.6f, 1.0f, 32, 0.0f);
        solid_cone(d, slot(3, 1, 0.2f), 2.6f, 1.2f, 32, 0.55f);
        solid_torus(d, slot(4, 1), 1.0f, 0.4f, 32, 16);
        solid_cone(d, slot(5, 1, 0.2f), 2.0f, 1.5f, 4, 0.1f); // pyramid

        // Outline on top of the box: slightly larger to avoid z-fighting
        d.set_depth_test(debug_depth_test_mode::depth_write);
        wire_box(d, slot(0, 1), vec3(1.004f, 1.004f, 1.004f), rgba(1.0f, 1.0f, 1.0f));
    }

    // -------------------------------------------- polyhedra, bounds, camera

    inline void showcase_polyhedra(debug_renderer& d)
    {
        reset_state(d);
        d.set_depth_test(debug_depth_test_mode::depth_write);
        solid_tetra(d, slot(0, 2), 1.4f);
        solid_octa(d, slot(1, 2), 1.3f);
        solid_icosa(d, slot(2, 2), 1.3f);

        d.set_depth_test(debug_depth_test_mode::depth_write);
        wire_tetra(d, slot(0, 2), 1.404f, rgba(1.0f, 1.0f, 1.0f));
        wire_octa(d, slot(1, 2), 1.304f, rgba(1.0f, 1.0f, 1.0f));
        wire_icosa(d, slot(2, 2), 1.304f, rgba(1.0f, 1.0f, 1.0f));

        // Camera frustum: looks along -Y, slightly down, Z is up
        frustum(d, slot(3, 2) + vec3(0.0f, 1.6f, 0.0f), vec3(0.0f, -1.0f, -0.1f), unit_z(), 0.87f, 1.6f, 0.4f, 2.4f, rgba(1.0f, 0.85f, 0.2f), rgba(0.6f, 0.5f, 0.1f));

        // AABB: corners, center and half extents
        {
            const vec3 c = slot(4, 2);
            const vec3 h(1.0f, 0.5f, 0.7f); // half sizes along X, Y, Z
            wire_box(d, c, h, hsv(0.6f, 0.5f));

            vec3 p[8];
            box_corners(p, c, h);
            d.set_point_size(7.0f);
            for (int i = 0; i < 8; ++i) {
                d.point(p[i], hsv(static_cast<float>(i) / 8.0f));
            }
            d.set_point_size(9.0f);
            d.point(c, rgba(1.0f, 1.0f, 0.3f));
            d.set_point_size(1.0f);

            d.line(c, c + vec3(h.x, 0.0f, 0.0f), rgba(1.0f, 0.2f, 0.2f));
            d.line(c, c + vec3(0.0f, h.y, 0.0f), rgba(0.2f, 1.0f, 0.2f));
            d.line(c, c + vec3(0.0f, 0.0f, h.z), rgba(0.3f, 0.5f, 1.0f));
        }

        // Normals visualization ("hedgehog")
        {
            const vec3 c = slot(5, 2);
            wire_sphere(d, c, 1.2f, 8, 12, rgba(0.3f, 0.3f, 0.35f));
            for (uint32 i = 1; i < 8; ++i) {
                for (uint32 j = 0; j < 12; ++j) {
                    const float theta = k_pi * fl(i) / 8.0f;
                    const float phi = k_two_pi * fl(j) / 12.0f;
                    const vec3  p = sphere_point(c, 1.2f, theta, phi);
                    const vec3  n = norm3(p - c);
                    d.line(p, hsv(theta / k_pi * 0.7f), p + n * 0.35f, rgba(1.0f, 1.0f, 1.0f));
                }
            }
        }
    }

    // ------------------------------------------------------------------ curves

    inline void showcase_curves(debug_renderer& d, float time)
    {
        reset_state(d);

        // Bezier with the control polygon
        {
            const vec3 o = slot(0, 3, 0.0f);
            const vec3 p0 = o + vec3(-1.6f, 0.0f, 0.2f), p1 = o + vec3(-1.6f, 0.8f, 3.0f);
            const vec3 p2 = o + vec3(1.6f, -0.8f, -0.5f), p3 = o + vec3(1.6f, 0.0f, 2.6f);
            bezier3(d, p0, p1, p2, p3, 48, 0.0f, 0.8f);

            const col grey = rgba(0.55f, 0.55f, 0.6f);
            d.line(p0, p1, grey);
            d.line(p1, p2, grey);
            d.line(p2, p3, grey);
            d.set_point_size(9.0f);
            d.point(p0, rgba(0.3f, 1.0f, 0.3f));
            d.point(p1, rgba(1.0f, 1.0f, 0.3f));
            d.point(p2, rgba(1.0f, 1.0f, 0.3f));
            d.point(p3, rgba(1.0f, 0.3f, 0.3f));
            d.set_point_size(1.0f);
        }

        torus_knot(d, slot(1, 3), 0.45f, 2, 3, 400);
        dna(d, slot(2, 3, 0.0f), 3.2f, 0.8f, 2.5f, 120);

        // Lorenz attractor: its z axis is our Z, no remapping needed
        {
            const vec3  o = slot(3, 3, 0.3f);
            const float s = 0.055f;
            float       x = 1.0f, y = 1.0f, z = 1.0f;
            const float dt = 0.005f;
            auto        step = [&] {
                const float dx = 10.0f * (y - x);
                const float dy = x * (28.0f - z) - y;
                const float dz = x * y - (8.0f / 3.0f) * z;
                x += dx * dt;
                y += dy * dt;
                z += dz * dt;
            };
            for (int i = 0; i < 300; ++i) {
                step();
            }
            const uint32 n = 3500;
            vec3         prev = o + vec3(x, y, z) * s;
            for (uint32 i = 0; i < n; ++i) {
                step();
                const vec3 cur = o + vec3(x, y, z) * s;
                const col  cl = hsv(fl(i) / fl(n) * 0.85f);
                d.line(prev, cl, cur, cl);
                prev = cur;
            }
        }

        // Waypoint path: green start, red end, arrows show direction
        {
            const vec3 o = slot(4, 3, 0.2f);
            const vec3 w[6] = {
                o + vec3(-1.8f, 1.0f, 0.0f), o + vec3(-1.0f, -0.5f, 0.4f), o + vec3(0.0f, 0.8f, 0.0f),
                o + vec3(0.9f, -0.6f, 0.5f), o + vec3(1.6f, 0.5f, 0.2f), o + vec3(1.9f, -0.4f, 1.4f)
            };
            for (int i = 0; i < 5; ++i) {
                arrow(d, w[i], w[i + 1], hsv(0.35f - 0.07f * static_cast<float>(i)), 0.22f, 0.07f);
            }
            d.set_point_size(9.0f);
            for (int i = 0; i < 6; ++i) {
                d.point(w[i], i == 0 ? rgba(0.3f, 1.0f, 0.3f) : (i == 5 ? rgba(1.0f, 0.3f, 0.3f) : rgba(1.0f, 1.0f, 0.3f)));
            }
            d.set_point_size(1.0f);
        }

        skeleton(d, slot(5, 3, 0.0f), 1.3f);

        // A small clock hand on the floor to see that the time works
        {
            const vec3  c = slot(5, 3, 0.1f) + vec3(-1.6f, 0.0f, 0.0f);
            const float a = time * 1.0f;
            arrow(d, c, c + vec3(std::cos(a) * 0.6f, std::sin(a) * 0.6f, 0.0f), rgba(1.0f, 1.0f, 1.0f), 0.15f, 0.05f);
            circle(d, c, unit_x(), unit_y(), 0.6f, 40, rgba(0.5f, 0.5f, 0.55f));
        }
    }

    // ------------------------------------------------------------------ points

    inline void showcase_points(debug_renderer& d, float time)
    {
        reset_state(d);

        // Fibonacci sphere
        {
            const vec3   o = slot(0, 4);
            const uint32 n = 600;
            const float  golden = k_pi * (3.0f - std::sqrt(5.0f));
            d.set_point_size(4.0f);
            for (uint32 i = 0; i < n; ++i) {
                const float z = 1.0f - 2.0f * (fl(i) + 0.5f) / fl(n);
                const float r = std::sqrt(1.0f - z * z);
                const float a = golden * fl(i);
                d.point(o + vec3(std::cos(a) * r, std::sin(a) * r, z) * 1.3f, hsv(fl(i) / fl(n)));
            }
        }

        // RGB color cube: position encodes color
        {
            const vec3   o = slot(1, 4);
            const uint32 n = 7;
            d.set_point_size(7.0f);
            for (uint32 i = 0; i < n; ++i) {
                for (uint32 j = 0; j < n; ++j) {
                    for (uint32 k = 0; k < n; ++k) {
                        const float r = fl(i) / fl(n - 1), g = fl(j) / fl(n - 1), b = fl(k) / fl(n - 1);
                        d.point(o + vec3(r - 0.5f, g - 0.5f, b - 0.5f) * 2.4f, rgba(r, g, b));
                    }
                }
            }
        }

        // Spiral galaxy: a disc in the XY plane
        {
            const vec3   o = slot(2, 4);
            const uint32 n = 2500;
            for (uint32 i = 0; i < n; ++i) {
                const float r = std::sqrt(hash01(i * 3 + 1)) * 2.0f;
                const float noise = (hash01(i * 3 + 2) - 0.5f) * 0.6f;
                const float arm = fl(i % 2) * k_pi;
                const float a = r * 2.2f + arm + noise + time * 0.25f;
                const float h = (hash01(i * 3 + 3) - 0.5f) * 0.3f * (1.0f - r / 2.0f);
                d.set_point_size(1.5f + (1.0f - r / 2.0f) * 3.0f);
                d.point(o + vec3(r * std::cos(a), r * std::sin(a), h), hsv(0.62f - r * 0.25f, 0.55f, 1.0f - r * 0.25f));
            }
        }

        // Solar system: orbits lie in the XY plane
        {
            const vec3 o = slot(4, 4);
            struct planet_t
            {
                float orbit, speed, size, hue;
            };
            const planet_t planets[4] = {{0.8f, 2.4f, 6.0f, 0.08f}, {1.3f, 1.7f, 8.0f, 0.55f}, {1.9f, 1.1f, 9.0f, 0.33f}, {2.6f, 0.6f, 12.0f, 0.02f}};

            d.set_depth_test(debug_depth_test_mode::depth_write);
            solid_sphere(d, o, 0.35f, 10, 16, [](float, float) { return rgba(1.0f, 0.85f, 0.2f); });
            d.set_depth_test(debug_depth_test_mode::depth_write);

            for (const auto& p : planets) {
                circle(d, o, unit_x(), unit_y(), p.orbit, 64, rgba(0.35f, 0.35f, 0.4f));
                d.set_point_size(p.size);
                d.point(ring_point(o, p.orbit, time * p.speed), hsv(p.hue, 0.7f));
            }

            // Moon around the third planet
            const vec3 earth = ring_point(o, planets[2].orbit, time * planets[2].speed);
            circle(d, earth, unit_x(), unit_y(), 0.25f, 24, rgba(0.35f, 0.35f, 0.4f));
            d.set_point_size(4.0f);
            d.point(ring_point(earth, 0.25f, time * 5.0f), rgba(0.85f, 0.85f, 0.85f));
            d.set_point_size(1.0f);
        }
    }

    // ----------------------------------------------- surfaces, fields, lights

    inline void showcase_wave(debug_renderer& d, float time)
    {
        reset_state(d);
        constexpr int n = 36;
        const float   half = 4.0f;
        const vec3    o(-9.0f, -6.0f, 0.8f);

        // Height field: z = f(x, y)
        auto height = [&](float x, float y) { return 0.45f * std::sin(x * 1.3f + time * 1.5f) * std::cos(y * 1.1f - time * 1.1f); };
        auto P = [&](int i, int j) {
            const float x = -half + 2.0f * half * static_cast<float>(i) / static_cast<float>(n);
            const float y = -half + 2.0f * half * static_cast<float>(j) / static_cast<float>(n);
            return vec3(o.x + x, o.y + y, o.z + height(x, y));
        };
        auto C = [&](vec3 p) { return hsv(0.62f - (p.z - o.z + 0.45f) / 0.9f * 0.55f, 0.85f, 1.0f); };

        d.set_depth_test(debug_depth_test_mode::depth_write);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                const vec3 A = P(i, j), B = P(i + 1, j), Cc = P(i + 1, j + 1), D = P(i, j + 1);
                d.triangle(A, C(A), B, C(B), D, C(D)); // faces +Z
                d.triangle(B, C(B), Cc, C(Cc), D, C(D));
            }
        }

        // Grid on top (lifted a bit to avoid z-fighting)
        d.set_depth_test(debug_depth_test_mode::depth_write);
        const vec3 lift(0.0f, 0.0f, 0.012f);
        const col  wire = rgba(1.0f, 1.0f, 1.0f, 0.25f);
        for (int i = 0; i <= n; ++i) {
            for (int j = 0; j <= n; ++j) {
                if (i < n) {
                    d.line(P(i, j) + lift, P(i + 1, j) + lift, wire);
                }
                if (j < n) {
                    d.line(P(i, j) + lift, P(i, j + 1) + lift, wire);
                }
            }
        }
    }

    inline void showcase_lights(debug_renderer& d, float time)
    {
        reset_state(d);

        // Point light: position, range as three circles
        {
            const vec3 c = slot(2, -1, 1.5f);
            const col  cl = rgba(1.0f, 0.85f, 0.4f);
            d.set_point_size(12.0f);
            d.point(c, rgba(1.0f, 0.95f, 0.6f));
            d.set_point_size(1.0f);
            circle(d, c, unit_x(), unit_y(), 1.6f, 48, cl);
            circle(d, c, unit_y(), unit_z(), 1.6f, 48, cl);
            circle(d, c, unit_x(), unit_z(), 1.6f, 48, cl);
        }

        // Spot light: cone with direction
        {
            const vec3 apex = slot(3, -1, 3.0f);
            const vec3 dir(0.0f, 0.15f * std::sin(time), -1.0f);
            wire_cone_dir(d, apex, dir, 2.8f, 0.5f, 40, rgba(0.6f, 0.9f, 1.0f));
            d.set_point_size(10.0f);
            d.point(apex, rgba(1.0f, 1.0f, 1.0f));
            d.set_point_size(1.0f);
        }

        // Directional light: a sun icon and parallel rays
        {
            const vec3 c = slot(4, -1, 3.0f);
            const col  cl = rgba(1.0f, 0.9f, 0.3f);
            circle(d, c, unit_x(), unit_y(), 0.4f, 24, cl);
            const vec3 dir = norm3(vec3(0.3f, 0.2f, -1.0f));
            for (int i = -2; i <= 2; ++i) {
                const vec3 from = c + vec3(static_cast<float>(i) * 0.5f, 0.0f, -0.5f);
                arrow(d, from, from + dir * 2.2f, cl, 0.25f, 0.07f);
            }
        }

        // Vector field: rotation around the Z axis, in a horizontal plane
        {
            const vec3 o = slot(5, -1, 0.3f);
            for (int ix = -3; ix <= 3; ++ix) {
                for (int iy = -3; iy <= 3; ++iy) {
                    const float x = static_cast<float>(ix) * 0.35f, y = static_cast<float>(iy) * 0.35f;
                    const float mag = std::sqrt(x * x + y * y);
                    if (mag < 0.1f) {
                        continue;
                    }
                    const vec3 p = o + vec3(x, y, 0.0f);
                    const vec3 dir = norm3(vec3(-y, x, 0.0f));
                    arrow(d, p, p + dir * (0.15f + mag * 0.25f), hsv(0.65f - mag * 0.5f), 0.1f, 0.035f);
                }
            }
        }
    }

    // ------------------------------------------------------------ transparency

    inline void showcase_transparent(debug_renderer& d)
    {
        reset_state(d);

        // Nested glass cubes
        {
            const vec3 c = slot(0, -2);
            col        g[6];
            rainbow_faces(g, 0.18f);
            solid_box(d, c, vec3(1.2f, 1.2f, 1.2f), g);
            rainbow_faces(g, 0.28f);
            solid_box(d, c, vec3(0.8f, 0.8f, 0.8f), g);
            rainbow_faces(g, 0.45f);
            solid_box(d, c, vec3(0.4f, 0.4f, 0.4f), g);
            wire_box(d, c, vec3(1.2f, 1.2f, 1.2f), rgba(1.0f, 1.0f, 1.0f, 0.6f));
        }

        // Overlapping RGB spheres (a triangle on the floor plane)
        {
            const vec3 c = slot(1, -2);
            const vec3 off[3] = {vec3(-0.5f, 0.3f, 0.0f), vec3(0.5f, 0.3f, 0.0f), vec3(0.0f, -0.55f, 0.0f)};
            const col  cl[3] = {rgba(1.0f, 0.1f, 0.1f, 0.35f), rgba(0.1f, 1.0f, 0.1f, 0.35f), rgba(0.2f, 0.3f, 1.0f, 0.35f)};
            for (int i = 0; i < 3; ++i) {
                solid_sphere(d, c + off[i], 1.0f, 14, 20, [&](float, float) { return cl[i]; });
            }
        }

        // Water plane with an alpha gradient (horizontal, faces +Z)
        {
            const vec3 c = slot(2, -2, 0.1f);
            const col  deep = rgba(0.1f, 0.35f, 1.0f, 0.55f), shallow = rgba(0.1f, 0.9f, 1.0f, 0.15f);
            quad_fill(
                d,
                c + vec3(-2.0f, -1.5f, 0.0f), deep, c + vec3(2.0f, -1.5f, 0.0f), deep,
                c + vec3(2.0f, 1.5f, 0.0f), shallow, c + vec3(-2.0f, 1.5f, 0.0f), shallow
            );
        }

        // X-ray: an object behind a wall is visible through it as a dim ghost
        {
            const vec3 wall = slot(4, -2);
            const vec3 obj = wall + vec3(0.0f, -2.0f, 0.0f);

            d.set_depth_test(debug_depth_test_mode::depth_write);
            col w[6];
            for (auto& c : w) {
                c = rgba(0.25f, 0.27f, 0.3f);
            }
            solid_box(d, wall, vec3(1.8f, 0.15f, 1.5f), w);      // a thin wall in the XZ plane

            d.set_depth_test(debug_depth_test_mode::depth_write); // visible parts
            wire_sphere(d, obj, 1.0f, 8, 12, hsv(0.1f));

            d.set_depth_test(debug_depth_test_mode::none); // ghost through the wall
            wire_sphere(d, obj, 1.0f, 8, 12, hsv(0.1f, 1.0f, 1.0f, 0.25f));
        }
    }

    // ------------------------------------------------------------------- all

    inline void showcase_all(debug_renderer& d, float time)
    {
        showcase_reference(d);
        showcase_wireframes(d);
        showcase_solids(d);
        showcase_polyhedra(d);
        showcase_curves(d, time);
        showcase_points(d, time);
        showcase_wave(d, time);
        showcase_lights(d, time);
        showcase_transparent(d);
        reset_state(d);
    }

} // namespace tavros::renderer::dbg


#define TAV_FATAL_IF(expr, msg)      \
    do {                             \
        if ((expr)) {                \
            logger.fatal("{}", msg); \
            ::std::abort();          \
        }                            \
    } while (0)

class main_window : public app::render_app_base
{
public:
    main_window(tavros::core::string_view name, tavros::core::shared_ptr<tavros::assets::asset_manager> am)
        : app::render_app_base(name)
        , m_am(am)
    {
        class tavros_engine_file_provider : public tavros::tef::source_provider
        {
        public:
            explicit tavros_engine_file_provider(tavros::core::shared_ptr<tavros::assets::asset_manager> am)
                : m_am(std::move(am))
            {
            }

            tavros::core::string load(tavros::core::string_view path) override
            {
                return m_am->read_text(path);
            }

        private:
            tavros::core::shared_ptr<tavros::assets::asset_manager> m_am;
        };

        auto loader = tavros::tef::loader(std::move(tavros::core::make_unique<tavros_engine_file_provider>(am)));
        auto config = loader.load("config.tef");
        m_ws = std::move(config);
        TAV_FATAL_IF(!m_ws, "falied to load config.tef");
    }

    ~main_window() override = default;

    static constexpr tavros::math::vec4 to_vec4(tavros::math::color32 rgba) noexcept
    {
        return tavros::math::vec4(
            static_cast<float>(rgba.r) / 255.0f,
            static_cast<float>(rgba.g) / 255.0f,
            static_cast<float>(rgba.b) / 255.0f,
            static_cast<float>(rgba.a) / 255.0f
        );
    }

    static constexpr tavros::math::color32 to_rgba8(const tavros::math::vec4& color) noexcept
    {
        auto clamp01 = [](float v) noexcept -> float {
            return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        };

        auto to_u8 = [&](float v) noexcept -> uint8 {
            return static_cast<uint8>(clamp01(v) * 255.0f + 0.5f);
        };

        return tavros::math::color32{
            to_u8(color.x),
            to_u8(color.y),
            to_u8(color.z),
            to_u8(color.w),
        };
    }

    static tavros::math::color32 rainbow_color(float time_seconds) noexcept
    {
        static tavros::math::vec4 colors[] = {
            to_vec4(tavros::math::color32(0xFF5959FF)), // Red
            to_vec4(tavros::math::color32(0xFF599BFF)), // Orange
            to_vec4(tavros::math::color32(0xFF59FFFF)), // Yellow
            to_vec4(tavros::math::color32(0xFF59FF59)), // Green
            to_vec4(tavros::math::color32(0xFFFFFF59)), // Blue
            to_vec4(tavros::math::color32(0xFFFF5959)), // Indigo
            to_vec4(tavros::math::color32(0xFFE859FF))  // Violet
        };
        constexpr size_t count = sizeof(colors) / sizeof(colors[0]);

        // Заворачиваем время в диапазон [0, count)
        float t = std::fmod(time_seconds, static_cast<float>(count));
        if (t < 0.0f) {
            t += static_cast<float>(count);
        }

        size_t index_a = static_cast<size_t>(t);
        size_t index_b = (index_a + 1) % count;
        float  frac = t - static_cast<float>(index_a);

        const tavros::math::vec4& a = colors[index_a];
        const tavros::math::vec4& b = colors[index_b];

        tavros::math::vec4 result{
            a.x + (b.x - a.x) * frac,
            a.y + (b.y - a.y) * frac,
            a.z + (b.z - a.z) * frac,
            a.w + (b.w - a.w) * frac,
        };

        return to_rgba8(result);
    }

    static constexpr tavros::math::color32 inv_color(tavros::math::color32 cl) noexcept
    {
        return tavros::math::color32(255 - cl.r, 255 - cl.g, 255 - cl.b, cl.a);
    }

    // ------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------

    void init() override
    {
        ZoneScopedNC("AppInit", 0x607D8B);
        init_graphics();
        show();
    }

    void shutdown() override
    {
        ZoneScopedNC("AppShutdown", 0x607D8B);
        m_ui = nullptr;
        m_renderer = nullptr;
    }

    // ------------------------------------------------------------------
    // Render loop
    // ------------------------------------------------------------------

    float time_sec = 0.0f;
    void  render(tavros::input::event_args_queue_view events, std::chrono::microseconds time_us) override
    {
        ZoneScopedNC("Frame", 0xFFFFFF);

        process_input(events, time_us);

        m_ui->process_input(events);

        float delta_time_sec = static_cast<float>(time_us.count()) / 1'000'000.0f;
        time_sec += delta_time_sec;


        tavros::renderer::dbg::showcase_all(m_dbg_renderer, time_sec);


        m_renderer->begin_frame(time_us.count());
        m_renderer->add_data(tavros::renderer::fd_camera{m_free_cam.camera()});
        m_renderer->add_data(tavros::renderer::fd_graphics_settings{16});
        m_renderer->add_data(tavros::renderer::fd_debug_draw_data{m_dbg_renderer.take()});

        // auto smp = m_renderer->resource_manager()->sampler(static_cast<tavros::renderer::sampler_preset>(m_smp_index));
        // frame.commands().bind_pipeline(m_skybox_pipeline->gpu_pipeline());
        // auto tex = m_sky_textures[m_sky_index];
        // frame.commands().bind_shader_textures(rhi::texture_binding{tex->gpu_texture(), smp, 0});
        // frame.commands().draw(36);

        // draw_world_grid(frame.commands(), 0);
        // draw_world_axis({m_input_manager.window_size().width - 120.0f, 120.0f}, 90.0f);


        tavros::core::long_string lb_text;
        const auto                cam_pos = m_free_cam.camera().position();
        lb_text.fprint("Frame number: {}\nMouse pos: x={:.1f}   y={:.1f}\nCamera position: x={:.3f}   y={:.3f}   z={:.3f}", m_frame_number, m_input_manager.mouse_pos().x, m_input_manager.mouse_pos().y, cam_pos.x, cam_pos.y, cam_pos.z);


        m_label.set_text(lb_text);
        m_label.set_text_color(rainbow_color(time_sec));
        m_label.set_outline_color(inv_color(rainbow_color(time_sec)));


        auto tex123 = m_renderer->resource_manager()->fonts_texture();

        if (static_cast<int32>(time_sec) % 10 < 5) {
            m_label.set_font_size(36.0f);
        } else {
            m_label.set_font_size(72.0f);
        }

        m_ui->resolve();
        m_ui->layout({0.0f, 0.0f, static_cast<float>(m_input_manager.window_size().width), static_cast<float>(m_input_manager.window_size().height)});
        m_ui->update(delta_time_sec);
        // m_ui->draw();

        m_renderer->end_frame();

        m_input_manager.end_frame();

        ++m_frame_number;

        FrameMark;
    }

private:
    // ==================================================================
    // Init helpers
    // ==================================================================

    void init_graphics()
    {
        ZoneScopedNC("InitGraphics", 0x607D8B);

        m_renderer = tavros::core::make_shared<tavros::renderer::render_system>(m_am, m_ws, native_handle());
        TAV_FATAL_IF(!m_renderer, "Failed to create render system.");

        m_free_cam.set_orientation(tavros::math::quat::look_rotation(tavros::math::vec3(-1.0f, -1.0f, -0.5f), tavros::math::vec3(0.0f, 0.0f, 1.0f)));
        m_free_cam.set_orbit_dist(30.0f);

        auto* rm = m_renderer->resource_manager();


        m_sky_textures.push_back(rm->load_texture("tex.cloudy_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.misty_morning"));
        // m_sky_textures.push_back(rm->load_texture("tex.cloudy_sunset_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.dark_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.pure_sunset_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.pure_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.sunset_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.rock_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.rock_sky_small"));
        m_sky_textures.push_back(rm->load_texture("tex.test"));

        m_ui_texture = rm->load_texture("tex.ui_actor_portrets");
        m_ui_texture2 = rm->load_texture("tex.ui_pda2_noice");

        rm->set_material_load_params({}, 1, rhi::pixel_format::depth32f);
        m_skybox_pipeline = rm->load_material("mt.skybox");
        m_world_grid_pipeline = rm->load_material("mt.world_grid");
        rm->set_material_load_params({}, 1, rhi::pixel_format::none);

        m_font = rm->load_font("fnt.consola_mono");
        // m_font = rm->load_font("fnt.droid_sans");
        m_font = rm->load_font("fnt.home_video");
        // m_font = rm->load_font("fnt.noto_sans_regular");
        // m_font = rm->load_font("fnt.roboto_medium");

        m_ui = tavros::core::make_unique<tavros::ui::root_view>(m_renderer);
        m_ui->set_root(&m_root_view);
        m_root_view.set_padding({20.0f, 30.0f, 20.0f, 50.0f});
        m_root_view.set_content_orientation(tavros::ui::content_orient::vertical);
        m_horiz_view.set_content_orientation(tavros::ui::content_orient::horizontal);

        m_root_view.insert_last_child(&m_view1);
        m_root_view.add_child(&m_label);
        m_root_view.add_child(&m_label2);
        m_root_view.insert_last_child(&m_view2);
        m_root_view.insert_last_child(&m_horiz_view);
        m_root_view.insert_last_child(&m_view3);
        m_root_view.set_border(10);
        m_horiz_view.add_child(&m_label3);
        m_horiz_view.add_child(&m_label4);
        // m_root_view.insert_last_child(&m_view4);

        m_view1.set_size({500, 200});
        m_view1.set_margin({30, 60, 90, 120});
        m_view1.set_padding({40, 60, 80, 100});
        m_view1.set_vertical_alignment(tavros::ui::v_align::center);

        m_view2.set_size({200, 150});
        m_view2.set_margin({20, 10, 20, 10});
        m_view2.set_padding({20, 10, 30, 10});
        m_view2.set_vertical_alignment(tavros::ui::v_align::center);

        m_view3.set_size({100, 300});
        m_view3.set_margin({10, 10, 10, 10});
        m_view3.set_padding({50, 50, 50, 50});

        // m_view4.set_size({150, 250});
        // m_view4.set_margin({0, 10, 20, 0});
        // m_view4.set_padding({100, 0, 100, 00,});


        m_horiz_view.insert_last_child(&m_view5);
        m_horiz_view.insert_last_child(&m_view6);
        m_horiz_view.insert_last_child(&m_view7);

        m_horiz_view.set_margin({10, 10, 10, 10});

        m_view5.set_size({50, 50});
        m_view5.set_padding({10, 10, 10, 10});
        m_view5.set_vertical_alignment(tavros::ui::v_align::center);

        m_view6.set_margin({10, 20, 60, 80});
        m_view6.set_size({100, 200});
        m_view6.set_padding({20, 20, 20, 20});
        m_view6.set_vertical_alignment(tavros::ui::v_align::center);

        m_view7.set_size({300, 400});
        m_view7.set_margin({30, 30, 30, 30});

        m_label.set_font("fnt.consola_mono");
        m_label.set_font_size(48.0f);
        m_label.set_margin(45.0f);
        m_label.set_padding({125.0f, 25.0f, 125.0f, 25.0f});
        m_label.set_horizontal_alignment(tavros::ui::h_align::center);
        m_label.set_min_size({300.0f, 0});
        // m_label.set_max_size({400.0f, tavros::ui::k_max_ui_size});
        m_label.set_border(5);
        m_label.set_border_radius(10);
        m_label.set_word_wrap_enabled(true);
        m_label.set_size({150000.0f, 0});

        m_label2.set_font("fnt.noto_sans_regular");
        m_label2.set_font_size(44.0f);
        m_label2.set_text("Hello world!!!");
        m_label2.set_padding(10.0f);
        m_label2.set_margin(10.0f);
        m_label2.set_horizontal_alignment(tavros::ui::h_align::right);

        m_label3.set_font("fnt.home_video");
        m_label3.set_font_size(55.0f);
        m_label3.set_text("Label 3. Hello world!");
        m_label3.set_padding(10.0f);
        m_label3.set_margin(10.0f);

        m_label4.set_font("fnt.home_video");
        m_label4.set_font_size(55.0f);
        m_label4.set_text("Label 4. Hello label.");
        m_label4.set_padding(10.0f);
        m_label4.set_margin(10.0f);
        m_label4.set_horizontal_alignment(tavros::ui::h_align::center);
    }

    // ==================================================================
    // Per-frame helpers
    // ==================================================================

    void process_input(tavros::input::event_args_queue_view events, std::chrono::microseconds time_us)
    {
        ZoneScopedNC("InputProcessEvents", 0x9C27B0);
        m_input_manager.begin_frame(tavros::system::application::instance().highp_time_us());
        m_input_manager.process_events(events);
        handle_window_resize();
        m_free_cam.update(m_input_manager, time_us);

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_equal)) {
            m_sky_index += 1;
            if (m_sky_index >= m_sky_textures.size()) {
                m_sky_index = 0;
            }
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_minus)) {
            m_sky_index -= 1;
            if (m_sky_index < 0) {
                m_sky_index = m_sky_textures.size() - 1;
            }
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_pagedown)) {
            m_smp_index += 1;
            if (m_smp_index >= static_cast<int32>(tavros::renderer::sampler_preset::count)) {
                m_smp_index = 0;
            }
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_1)) {
            m_label.set_font("asdafadfasdfaerwtqgsafv");
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_2)) {
            m_label.set_font("fnt.droid_sans");
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_3)) {
            m_label.set_font("fnt.home_video");
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_4)) {
            m_label.set_font("fnt.noto_sans_regular");
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_5)) {
            m_label.set_font("fnt.roboto_medium");
        }
    }

    void handle_window_resize()
    {
        if (!m_input_manager.is_window_resized()) {
            return;
        }
        const auto sz = m_input_manager.window_size();
        if (sz.width == 0 || sz.height == 0) {
            return;
        }
        TracyMessageL("WindowResize");

        m_renderer->resize_backbuffer(sz.width, sz.height);
        const float fov_y = 70.0f * 3.14159265358979f / 180.0f;
        const float near = 0.1f;
        const float far = 5000.0f;
        m_free_cam.set_perspective(fov_y, static_cast<float>(sz.width) / static_cast<float>(sz.height), near, far);
    }

    void draw_world_axis(const tavros::math::vec2& pos, float radius)
    {
        using namespace tavros;
        float line_width = 6.0f;
        float dash_size = 10.0f;
        float gap_size = 5.0f;
        float p_side_circle = 19.0f;
        float n_side_circle = 13.0f;
        float axis_len = radius - p_side_circle - line_width;

        auto view3 = math::mat3(m_free_cam.camera().view_matrix());

        auto project_axis = [&](math::vec3 world_dir) -> math::vec2 {
            math::vec3 cam = view3 * world_dir;
            return math::vec2(cam.x, -cam.y) * axis_len;
        };

        auto depth_of = [&](math::vec3 world_dir) -> float {
            return (view3 * world_dir).z;
        };

        // depth в диапазоне [-1..1] -> alpha [0.2..1.0]
        auto depth_to_alpha = [](float depth) -> uint8 {
            if (depth > -0.2) {
                return 255;
            }
            float t = (depth + 1.2f);  // [-1..-0.2] -> [0..1]
            t = std::clamp(t, 0.0f, 1.0f);
            float a = 0.1f + t * 0.9f; // [0.2..1.0]
            return static_cast<uint8>(t * 255.0f);
        };

        auto with_alpha = [](math::color32 color, uint8 alpha) -> math::color32 {
            return {color.r, color.g, color.b, alpha};
        };

        constexpr auto x_color = math::color32(0xFF533AA7);
        constexpr auto y_color = math::color32(0xFF23B06B);
        constexpr auto z_color = math::color32(0xFFBE833A);

        struct axis_info
        {
            math::vec2    dir;
            math::color32 color;
            float         depth;
            char          pos_label[3];
            char          neg_label[3];
            float         pos_font_size;
            float         neg_font_size;
        };

        axis_info axes[3] = {
            {project_axis({1, 0, 0}), x_color, depth_of({1, 0, 0}), "X", "-X", 36.0f, 22.0f},
            {project_axis({0, 1, 0}), y_color, depth_of({0, 1, 0}), "Y", "-Y", 36.0f, 22.0f},
            {project_axis({0, 0, 1}), z_color, depth_of({0, 0, 1}), "Z", "-Z", 36.0f, 22.0f},
        };

        int order[3] = {0, 1, 2};
        std::sort(std::begin(order), std::end(order), [&](int a, int b) {
            return axes[a].depth > axes[b].depth;
        });

        tavros::renderer::rich_text text;
        text.set_font(m_font);

        auto draw_axis_neg = [&](const axis_info& ax) {
            // Негативный конец — глубина противоположна
            uint8 alpha = depth_to_alpha(-ax.depth);
            auto  col = with_alpha(ax.color, alpha);

            // Линия: градиент от центра (полный цвет) до конца (с альфой)
            /*m_uir->set_brush_solid_color(col);
            m_uir->draw_line(pos, pos - ax.dir);*/

            // Круг и текст — solid с той же альфой
            m_uir->superellipse(pos - ax.dir, {n_side_circle, 0.0}, {0.0, n_side_circle}, 1.0)
                .fill_color(col)
                .draw();

            text.clear();
            text.set_font_size(ax.neg_font_size);
            text.append_text(ax.neg_label);
            auto l = text.layout();
            /*m_uir->set_brush_solid_color(col);
            m_uir->draw_text(text.text(), pos - ax.dir - l.center(), 0.5f, 0.0f);*/
        };

        auto draw_axis_pos = [&](const axis_info& ax) {
            uint8 alpha = depth_to_alpha(ax.depth);
            auto  col = with_alpha(ax.color, alpha);

            // Линия: градиент от центра (полный цвет) до конца (с альфой)
            m_uir->line(pos, pos + ax.dir, line_width)
                .fill_color(col)
                .draw();

            // Круг и текст — solid с той же альфой
            m_uir->superellipse(pos + ax.dir, {p_side_circle, 0.0}, {0.0, p_side_circle}, 1.0)
                .fill_color(col)
                .draw();

            text.clear();
            text.set_font_size(ax.pos_font_size);
            text.append_text(ax.pos_label);
            auto l = text.layout();
            /*m_uir->set_brush_solid_color(col);
            m_uir->draw_text(text.text(), pos + ax.dir - l.center(), 0.5f, 0.15f);*/
        };

        for (int i : order) {
            if (axes[i].depth > 0.0f) {
                draw_axis_neg(axes[i]);
            } else {
                draw_axis_pos(axes[i]);
            }
        }

        for (int i = 2; i >= 0; --i) {
            if (axes[order[i]].depth > 0.0f) {
                draw_axis_pos(axes[order[i]]);
            } else {
                draw_axis_neg(axes[order[i]]);
            }
        }
    }

    void draw_world_grid(rhi::command_queue& cbuf, uint32 plane = 0)
    {
        using namespace tavros;

        struct shader_data
        {
            uint32 minor_color = 0;
            uint32 major_color = 0;
            uint32 axis_u_color = 0;
            uint32 axis_v_color = 0;
            float  minor_line_width = 0.0f;
            float  major_line_width = 0.0f;
            float  axis_line_width = 0.0f;
            float  minor_grid_step = 0.0f;
            float  major_grid_step = 0.0f;
            uint32 flags = 0; // flags & 0x3 == (0 - plane XY; 1 - plane YZ; 2 - plane ZX)
        };

        uint32 u_color = 0;
        uint32 v_color = 0;

        constexpr auto x_axis_color = math::vec4(0.6549f, 0.2196f, 0.3255f, 1.0f);
        constexpr auto y_axis_color = math::vec4(0.4196f, 0.5569f, 0.1373f, 1.0f);
        constexpr auto z_axis_color = math::vec4(0.2314f, 0.5137f, 0.7412f, 1.0f);

        auto plane_id = plane & 0x3;
        if (plane_id == 0) { // XY plane
            u_color = math::color32(x_axis_color).packed;
            v_color = math::color32(y_axis_color).packed;
        } else if (plane_id == 1) { // YZ plane
            u_color = math::color32(y_axis_color).packed;
            v_color = math::color32(z_axis_color).packed;
        } else if (plane_id == 2) { // ZX plane
            u_color = math::color32(z_axis_color).packed;
            v_color = math::color32(x_axis_color).packed;
        }
        // TODO
        shader_data sd;
        sd.minor_color = math::color32(75, 75, 75, 76).packed;
        sd.major_color = math::color32(100, 100, 100, 76).packed;
        sd.axis_u_color = u_color;
        sd.axis_v_color = v_color;
        sd.minor_line_width = 1.0f;
        sd.major_line_width = 3.0f;
        sd.axis_line_width = 5.0f;
        sd.minor_grid_step = 10.0f;
        sd.major_grid_step = 100.0f;
        sd.flags = plane_id;

        cbuf.bind_pipeline(m_world_grid_pipeline->gpu_pipeline());
        cbuf.push_constant(sd);
        cbuf.draw(4);
    }


    // ==================================================================
    // Members
    // ==================================================================

    tavros::core::shared_ptr<tavros::tef::workspace>          m_ws;
    tavros::core::shared_ptr<tavros::assets::asset_manager>   m_am;
    tavros::core::shared_ptr<tavros::renderer::render_system> m_renderer;

    // -- Scene --
    tavros::sandbox::free_camera m_free_cam;

    tavros::core::vector<tavros::renderer::texture_ref> m_sky_textures;
    int32                                               m_sky_index = 0;
    int32                                               m_smp_index = 0;

    tavros::renderer::texture_ref m_ui_texture;
    tavros::renderer::texture_ref m_ui_texture2;

    tavros::renderer::material_ref m_world_grid_pipeline;
    tavros::renderer::material_ref m_skybox_pipeline;

    tavros::renderer::font_ref m_font;


    tavros::core::shared_ptr<tavros::renderer::ui_renderer> m_uir;

    // -- Input --
    tavros::input::input_manager m_input_manager;

    //
    uint64 m_frame_number = 0;

    tavros::core::unique_ptr<tavros::ui::root_view> m_ui;

    tavros::ui::linear_view m_root_view = tavros::ui::linear_view("root_view");
    tavros::ui::linear_view m_vert_view = tavros::ui::linear_view("vert_view");
    tavros::ui::linear_view m_horiz_view = tavros::ui::linear_view("horiz_view");
    tavros::ui::view        m_view1 = tavros::ui::view("view1");
    tavros::ui::view        m_view2 = tavros::ui::view("view2");
    tavros::ui::view        m_view3 = tavros::ui::view("view3");
    tavros::ui::view        m_view4 = tavros::ui::view("view4");
    tavros::ui::view        m_view5 = tavros::ui::view("view5");
    tavros::ui::view        m_view6 = tavros::ui::view("view6");
    tavros::ui::view        m_view7 = tavros::ui::view("view7");
    tavros::ui::view        m_view8 = tavros::ui::view("view8");
    tavros::ui::view        m_view9 = tavros::ui::view("view9");
    tavros::ui::label_view  m_label = tavros::ui::label_view("label");
    tavros::ui::label_view  m_label2 = tavros::ui::label_view("label2");
    tavros::ui::label_view  m_label3 = tavros::ui::label_view("label3");
    tavros::ui::label_view  m_label4 = tavros::ui::label_view("label4");

    tavros::renderer::debug_renderer m_dbg_renderer;
};

// -------------------------------------------------------------------------
// Entry point
// -------------------------------------------------------------------------

#include <iostream>

int main()
{
    tavros::core::logger::add_consumer(
        [](auto, auto, auto msg) { std::printf("%s\n", msg.data()); }
    );

    tavros::debug::set_debug_path(TAV_OUTPUT_PATH);

    auto am = tavros::core::make_shared<tavros::assets::asset_manager>();
    am->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH, "");
    am->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH, "file");
    am->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH "/shaders", "");
    am->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH "/shaders", "file");
    am->mount<tavros::assets::filesystem_provider>(TAV_OUTPUT_PATH, "output", false, true);

    auto wnd = tavros::core::make_unique<main_window>("TavrosEngine", am);
    wnd->run_render_loop();

    return tavros::system::application::instance().run();
}