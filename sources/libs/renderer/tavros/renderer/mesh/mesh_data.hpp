#pragma once

#include <tavros/renderer/components/all.hpp>
#include <tavros/core/geometry/aabb3.hpp>
#include <tavros/core/archetype.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/containers/vector.hpp>

namespace tavros::renderer
{

    /**
     * @brief Minimal vertex archetype for opaque lit geometry.
     *
     * Provides the three streams required by most standard pipelines:
     * world/object-space position, surface normal, and a single UV channel.
     *
     * Use as the default @p Archetype parameter for @ref basic_mesh_data
     * when no additional per-vertex data (tangents, vertex colors, etc.) is needed.
     *
     * @see basic_mesh_data
     * @see mesh_data
     */
    using simple_mesh_archetype = core::basic_archetype<
        position_c,
        normal_c,
        uv0_c>;


    template<core::archetype_with<position_c> Archetype>
    struct basic_mesh_data
    {
        using archetype_type = Archetype;

        /** @brief The archetype type used for per-vertex data storage. */
        archetype_type vertices;

        /**
         * @brief Per-vertex data stored as a SoA archetype.
         *
         * Each component type in @p Archetype maps to one contiguous array.
         * Access individual streams via @c vertices.view<ComponentType>().
         */
        core::vector<uint32> indices;

        /**
         * Axis-aligned bounding box in the mesh's local space.
         * Computed once on import; used for culling and picking.
         */
        geometry::aabb3 bounds;


        /** Returns the number of vertices stored in the archetype. */
        [[nodiscard]] size_t vertex_count() const noexcept
        {
            return vertices.size();
        }

        /** Returns the number of indices (not triangles). */
        [[nodiscard]] size_t index_count() const noexcept
        {
            return indices.size();
        }

        /** Returns the number of triangles (index_count / 3). */
        [[nodiscard]] size_t triangle_count() const noexcept
        {
            return indices.size() / 3;
        }

        /** Returns true if this mesh contains no geometry. */
        [[nodiscard]] bool empty() const noexcept
        {
            return vertices.empty();
        }

        /**
         * Recalculates the AABB from the current position_c stream.
         * Call this after programmatically building or modifying vertex positions.
         */
        void recompute_bounds() noexcept
        {
            bounds = geometry::aabb3();
            vertices.view<position_c>().each([&](const position_c& p) {
                bounds.expand(p.value);
            });
        }
    };

    using mesh_data = basic_mesh_data<simple_mesh_archetype>;

} // namespace tavros::renderer
