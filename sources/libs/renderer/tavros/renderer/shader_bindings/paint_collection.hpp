#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/math/color32.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/renderer/shader_bindings/shader_binding_indices.hpp>

namespace tavros::renderer
{

    /** @brief A color stop used by gradients. */
    struct color_stop
    {
        /// Normalized stop position in the range [0, 1].
        float stop = 0.0f;

        /// Color at the stop position.
        math::color32 color;
    };

    /**
     * @brief View describing a paint stored in a paint_collection.
     *
     * References a contiguous range of color stops inside the collection.
     * The same view is uploaded to the GPU and interpreted by the shader.
     */
    struct paint_view
    {
        /// Index of the first color stop.
        uint32 first = 0;

        /// Number of color stops.
        uint32 number = 0;
    };

    /**
     * @brief Collection of paint definitions for GPU upload.
     *
     * Stores color stops in a single contiguous array. Each paint is represented
     * by a paint_view that references a subrange of this array. The memory layout
     * matches the layout expected by the shader, allowing direct upload without
     * additional processing.
     */
    class paint_collection
    {
    public:
        /** @brief Constructs an empty paint collection. */
        paint_collection() noexcept = default;

        /** @brief Default destructor. */
        ~paint_collection() noexcept = default;

        /**
         * @brief Adds a paint to the collection.
         *
         * The paint may contain a single color or multiple color stops defining
         * a gradient.
         *
         * @param color_stops Color stops describing the paint.
         * @return View referencing the stored paint.
         */
        paint_view add_paint(core::buffer_view<color_stop> color_stops);

        /** @brief Returns the number of bytes required for GPU upload. */
        size_t upload_size_bytes() const noexcept;

        /**
         * @brief Uploads the collection to a GPU stream buffer.
         * @param sb Destination stream buffer.
         * @return Buffer binding referencing the uploaded data.
         */
        rhi::buffer_binding upload(gpu_stream_buffer& sb) const noexcept;

        /**
         * @brief Returns all stored color stops.
         * @return View of the internal color stop array.
         */
        core::buffer_view<color_stop> paints() const noexcept;

        /** @brief Removes all stored paints. */
        void clear() noexcept;

    private:
        /**
         * @brief Header of the GPU paint buffer.
         *
         * This structure is mirrored 1:1 in the shader. It is immediately followed
         * by a variable-length array of color_stop elements.
         */
        struct paint_buffer
        {
            /// Number of color stops stored after the header.
            uint32 size = 0;

            /// Reserved for future use. Must be zero.
            uint32 reserved = 0;

            // color_stop paints[]; // Stored immediately after the header in GPU memory.
        };

    private:
        core::vector<color_stop> m_paints;
    };

} // namespace tavros::renderer
