#include <tavros/renderer/geometry/builtin_geometry_generator.hpp>

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/math.hpp>

namespace
{

    struct edge_key_t
    {
        uint32 a;
        uint32 b;
        edge_key_t(uint32 x, uint32 y) noexcept
        {
            if (x < y) {
                a = x;
                b = y;
            } else {
                a = y;
                b = x;
            }
        }
        bool operator==(edge_key_t const& o) const noexcept
        {
            return a == o.a && b == o.b;
        }
    };

    struct edge_hash_t
    {
        size_t operator()(edge_key_t const& k) const noexcept
        {
            // pack two 32-bit into 64-bit and hash
            uint64_t v = (static_cast<uint64_t>(k.a) << 32) | static_cast<uint64_t>(k.b);
            return std::hash<uint64_t>()(v);
        }
    };

    tavros::core::logger logger("builtin_geometry_generator");
} // namespace

namespace tavros::renderer
{

    builtin_geometry_info builtin_geometry_generator::cube_info()
    {
        builtin_geometry_info info;
        info.vertices_count = 24;
        info.indices_count = 36;
        return info;
    }

    bool builtin_geometry_generator::gen_cube(core::buffer_span<builtin_geometry_vertex> vertices, core::buffer_span<uint32> indices)
    {
        auto info = cube_info();
        if (info.vertices_count > vertices.size() || info.indices_count > indices.size()) {
            ::logger.error(
                "Failed to generate cube. "
                "Expected at least {} vertices and {} indices, but got {} vertices and {} indices.",
                fmt::styled_param(info.vertices_count), fmt::styled_param(info.indices_count),
                fmt::styled_param(vertices.size()), fmt::styled_param(indices.size())
            );
            return false;
        }

        static const builtin_geometry_vertex cube_vertices[] = {
            // +X (Front / forward)
            {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
            {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},

            // -X (Back / backward)
            {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
            {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
            {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
            {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},

            // +Y (Right)
            {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
            {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
            {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},

            // -Y (Left)
            {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
            {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
            {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}},
            {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}},

            // +Z (Up)
            {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},

            // -Z (Down)
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
            {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
            {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
            {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
        };

        const uint32 cube_indices[] = {
            0, 2, 1, 2, 0, 3,       // +X
            4, 6, 5, 6, 4, 7,       // -X
            8, 10, 9, 10, 8, 11,    // +Y
            12, 14, 13, 14, 12, 15, // -Y
            16, 18, 17, 18, 16, 19, // +Z
            20, 22, 21, 22, 20, 23  // -Z
        };

        vertices.copy_from(cube_vertices, info.vertices_count);
        indices.copy_from(cube_indices, info.indices_count);

        return true;
    }

    builtin_geometry_info builtin_geometry_generator::icosphere_info(uint32 subdivisions)
    {
        builtin_geometry_info info;

        // initial icosahedral sphere
        uint32 faces_number = 20;
        uint32 vertices_number = 12;

        for (uint32 i = 0; i < subdivisions; ++i) {
            faces_number *= 4;

            // Number of new vertices:
            // Each edge adds 1 new vertex,
            // But edges are divided between faces, so the formula is:
            // Vn = V + E
            // E = (3 * F) / 2 (for a triangular mesh)
            // After subdivision:
            // F' = 4 * F
            // E' = 3 * F'
            // But we must account for edge merging -> E'/2
            uint32 edges_number = (3 * faces_number) / 2;
            vertices_number = vertices_number + edges_number - faces_number + 2; // Euler's formula for spheres: V - E + F = 2
        }

        info.vertices_count = vertices_number;
        info.indices_count = faces_number * 3;
        return info;
    }

    bool builtin_geometry_generator::gen_icosphere(uint32 subdivisions, core::buffer_span<builtin_geometry_vertex> vertices, core::buffer_span<uint32> indices)
    {
        if (subdivisions > 12) {
            ::logger.error(
                "Failed to generate icosphere (subdivisions = {}). "
                "Subdivision level too high — potential integer overflow when computing vertex or index count.",
                fmt::styled_param(subdivisions)
            );
            return false;
        }

        auto info = icosphere_info(subdivisions);
        if (info.vertices_count > vertices.size() || info.indices_count > indices.size()) {
            ::logger.error(
                "Failed to generate icosphere (subdivisions = {}). "
                "Expected at least {} vertices and {} indices, but got {} vertices and {} indices.",
                fmt::styled_param(subdivisions),
                fmt::styled_param(info.vertices_count), fmt::styled_param(info.indices_count),
                fmt::styled_param(vertices.size()), fmt::styled_param(indices.size())
            );
            return false;
        }

        const auto               t = math::k_phi<float>;
        core::vector<math::vec3> verts = {
            math::normalize(math::vec3{-1.0f, t, 0.0f}),
            math::normalize(math::vec3{1.0f, t, 0.0f}),
            math::normalize(math::vec3{-1.0f, -t, 0.0f}),
            math::normalize(math::vec3{1.0f, -t, 0.0f}),
            math::normalize(math::vec3{0.0f, -1.0f, t}),
            math::normalize(math::vec3{0.0f, 1.0f, t}),
            math::normalize(math::vec3{0.0f, -1.0f, -t}),
            math::normalize(math::vec3{0.0f, 1.0f, -t}),
            math::normalize(math::vec3{t, 0.0f, -1.0f}),
            math::normalize(math::vec3{t, 0.0f, 1.0f}),
            math::normalize(math::vec3{-t, 0.0f, -1.0f}),
            math::normalize(math::vec3{-t, 0.0f, 1.0f}),
        };

        struct face_t
        {
            uint32 v1, v2, v3;
        };

        core::vector<face_t> faces = {
            {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8}, {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
        };

        // subdivide with midpoint cache to avoid duplicate vertices
        for (uint32 i = 0; i < subdivisions; ++i) {
            core::unordered_map<edge_key_t, uint32, edge_hash_t> midpoint_cache;
            core::vector<face_t>                                 new_faces;
            new_faces.reserve(faces.size() * 4);

            auto get_midpoint_index = [&](uint32 a, uint32 b) -> uint32 {
                edge_key_t key(a, b);
                auto       it = midpoint_cache.find(key);
                if (it != midpoint_cache.end()) {
                    return it->second;
                }

                math::vec3 mid = math::normalize((verts[a] + verts[b]) * 0.5f);
                uint32     idx = static_cast<uint32>(verts.size());
                verts.push_back(mid);
                midpoint_cache.emplace(key, idx);
                return idx;
            };

            for (const auto& tri : faces) {
                uint32 a = get_midpoint_index(tri.v1, tri.v2);
                uint32 b = get_midpoint_index(tri.v2, tri.v3);
                uint32 c = get_midpoint_index(tri.v3, tri.v1);

                // preserve winding as v1,v2,v3 => children:
                new_faces.push_back({tri.v1, a, c});
                new_faces.push_back({tri.v2, b, a});
                new_faces.push_back({tri.v3, c, b});
                new_faces.push_back({a, b, c});
            }

            faces = std::move(new_faces);
        }

        // now we have unique vert_positions and faces (indices)
        for (size_t i = 0; i < verts.size(); ++i) {
            vertices[i].pos = verts[i];
            vertices[i].norm = verts[i];
        }

        size_t idx = 0;
        for (const auto& tri : faces) {
            indices[idx++] = tri.v1;
            indices[idx++] = tri.v2;
            indices[idx++] = tri.v3;
        }

        return true;
    }

} // namespace tavros::renderer
