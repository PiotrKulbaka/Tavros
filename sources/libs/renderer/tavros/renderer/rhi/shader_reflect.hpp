#pragma once

#include <tavros/core/fixed_string.hpp>
#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer::rhi
{

    /**
     * @brief Reflection data for a vertex attribute.
     * Describes a single input attribute of a vertex shader.
     */
    struct vertex_attribute_reflect
    {
        /// Attribute name in shader
        core::fixed_string<63> name;

        /// Value shape (only: scalar, vec2, vec3, vec4)
        composite_format format = composite_format::scalar;

        /// Base scalar type of attribute
        scalar_type type = scalar_type::f32;

        /// Number of elements (0 = not array)
        uint32 array_size = 0;

        /// Attribute location index
        uint32 location = 0;
    };

    /**
     * @brief Reflection data for a shader resource binding.
     * Represents samplers, images or buffers used by the shader.
     */
    struct shader_resource_reflect
    {
        /// Resource name in shader
        core::fixed_string<63> name;

        /// Type of shader resource
        shader_resource_type type;

        /// Binding point
        uint32 binding = 0;
    };

    /**
     * @brief Reflection data for a single member inside a constant block.
     * Used for introspection of constant buffer layouts.
     */
    struct member_reflect
    {
        /// Member name
        core::fixed_string<63> name;

        /// Data shape (scalar/vector)
        composite_format format = composite_format::scalar;

        /// Scalar element type
        scalar_type type = scalar_type::f32;

        /// Byte offset inside block
        uint32 offset = 0;

        /// Number of elements (0 = not array)
        uint32 array_size = 0;

        /// Stride between array elements (only if array_size != 0)
        uint32 array_stride = 0;
    };

    /**
     * @brief Reflection data for a constant buffer (UBO).
     */
    struct constant_block_reflect
    {
        /// Block name
        core::fixed_string<63> name;

        /// Total size in bytes
        uint32 size = 0;

        /// Binding point
        uint32 binding = 0;
    };

    /**
     * @brief Reflection data for a storage buffer (SSBO).
     */
    struct storage_block_reflect
    {
        /// Block name
        core::fixed_string<63> name;

        /// Total size in bytes
        uint32 size = 0;

        /// Binding point
        uint32 binding = 0;
    };

    /**
     * @brief Reflection data for a shader output variable.
     */
    struct output_reflect
    {
        /// Output variable name
        core::fixed_string<63> name;

        /// Output shape (only: scalar, vec2, vec3, vec4)
        composite_format format = composite_format::scalar;

        /// Output scalar type
        scalar_type type = scalar_type::f32;

        /// Output location index
        uint32 location = 0;
    };

    /**
     * @brief Compute shader reflection.
     */
    struct compute_reflect
    {
        /// Is compute shader.
        bool is_compute = false;

        /// Number of invocations in X dimension.
        uint32 local_size_x = 0;

        /// Number of invocations in Y dimension
        uint32 local_size_y = 0;

        /// Number of invocations in Z dimension.
        uint32 local_size_z = 0;
    };

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
