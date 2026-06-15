#pragma once

#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/renderer/rhi/structs.hpp>

namespace tavros::renderer::rhi
{

    /**
     * @brief Complete reflection interface for a shader program.
     *
     * Provides read-only access to all reflection data extracted from a shader program,
     * including inputs, outputs, resource bindings and buffer layouts.
     */
    class shader_reflect
    {
    public:
        /**
         * Default destructor.
         */
        virtual ~shader_reflect() noexcept = default;

        /**
         * @brief Returns all vertex input attributes used by the shader.
         */
        virtual core::buffer_view<vertex_attribute_reflect> vertex_attributes() const noexcept = 0;

        /**
         * @brief Returns all shader resource bindings.
         */
        virtual core::buffer_view<shader_resource_reflect> shader_resources() const noexcept = 0;

        /**
         * @brief Returns all constant buffer (UBO) declarations.
         */
        virtual core::buffer_view<constant_block_reflect> constant_blocks() const noexcept = 0;

        /**
         * @brief Returns members of a specific uniform buffer block.
         *
         * @param constant_block_index Index of the constant block in constant_blocks().
         *
         * @return Read-only view over member reflection data for the selected block.
         *
         * @note Index must be valid and refer to an existing constant block.
         */
        virtual core::buffer_view<member_reflect> constant_block_members(size_t constant_block_index) const noexcept = 0;

        /**
         * @brief Returns all storage buffer (SSBO) declarations.
         */
        virtual core::buffer_view<storage_block_reflect> storage_blocks() const noexcept = 0;

        /**
         * @brief Returns all shader output variables.
         */
        virtual core::buffer_view<output_reflect> outputs() const noexcept = 0;

        /**
         * @brief Returns compute shader specific info.
         */
        virtual const compute_reflect& compute() const noexcept = 0;
    };

} // namespace tavros::renderer::rhi
