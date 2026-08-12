#pragma once

#include <tavros/core/math.hpp>
#include <tavros/renderer/shader_bindings/paint_collection.hpp>

namespace tavros::renderer
{

    /** @brief Identifier of a brush stored in a brush_collection. */
    struct brush_id
    {
        /// Brush index.
        uint32 index = 0;
    };

    /**
     * @brief Collection of brushes for GPU upload.
     *
     * Stores solid and gradient brushes in a contiguous array. The collection is
     * uploaded to the GPU without additional conversion, and brush_id values
     * reference individual brush entries.
     */
    class brush_collection
    {
    public:
        /** @brief Constructs an empty brush collection. */
        brush_collection() noexcept = default;

        /** @brief Default destructor. */
        ~brush_collection() noexcept = default;

        /**
         * @brief Adds a solid color brush.
         * @param color Brush color.
         * @return Identifier of the created brush.
         */
        brush_id add_solid(math::color32 color);

        /**
         * @brief Adds a linear gradient brush.
         *
         * @param paint Gradient color stops.
         * @param from  Gradient start point.
         * @param to    Gradient end point.
         * @return Identifier of the created brush.
         */
        brush_id add_linear_gradient(paint_view paint, math::vec2 from, math::vec2 to);

        /**
         * @brief Adds a radial gradient brush.
         *
         * @param paint  Gradient color stops.
         * @param center Gradient center.
         * @param radius Gradient radius.
         * @return Identifier of the created brush.
         */
        brush_id add_radial_gradient(paint_view paint, math::vec2 center, float radius);

        /**
         * @brief Returns the number of bytes required for GPU upload.
         * @return Upload buffer size in bytes.
         */
        size_t upload_size_bytes() const noexcept;

        /**
         * @brief Uploads the collection to a GPU stream buffer.
         * @param sb Destination stream buffer.
         * @return Buffer binding referencing the uploaded data.
         */
        rhi::buffer_binding upload(gpu_stream_buffer& sb) const noexcept;

        /** @brief Removes all stored brushes. */
        void clear() noexcept;

    private:
        /** @brief GPU representation of a brush. This structure is mirrored 1:1 in the shader. */
        struct brush_type
        {
            /// Brush flags and type information.
            uint32 flags = 0;

            /// Index of the first gradient color stop.
            uint32 first_color_index = 0;

            /// Number of gradient color stops.
            uint32 number_colors = 0;

            /// Solid brush color. Used only when number_colors == 1.
            math::color32 color;

            /// Brush-specific parameter.
            math::vec2 arg0;

            /// Brush-specific parameter.
            math::vec2 arg1;
        };

        /**
         * @brief Header of the GPU brush buffer.
         * This structure is mirrored 1:1 in the shader. It is immediately followed
         * by a variable-length array of brush_type elements.
         */
        struct brush_buffer
        {
            /// Number of brushes stored after the header.
            uint32 size = 0;

            /// Color returned when no brush is bound.
            math::color32 no_brush_color;

            /// Reserved for future use. Must be zero.
            uint32 reserved0 = 0;

            /// Reserved for future use. Must be zero.
            uint32 reserved1 = 0;

            // brush_type brushes[]; // Stored immediately after the header in GPU memory.
        };

    private:
        core::vector<brush_type> m_brushes;
    };

} // namespace tavros::renderer
