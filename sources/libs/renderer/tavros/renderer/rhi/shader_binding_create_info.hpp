#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/limits.hpp>
#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Describes a texture + sampler binding to a shader stage
     */
    struct texture_binding
    {
        /// Handle to the texture resource to bind
        texture_handle texture;

        /// Handle to the sampler resource to bind
        sampler_handle sampler;

        /// Binding slot in the shader (matches `layout(binding=X)` in GLSL/HLSL)
        uint32 binding = 0;
    };

    /**
     * Describes a binding of a buffer or subrange of a buffer to a shader stage
     */
    struct buffer_binding
    {
        /// Handle to the buffer resource to bind
        buffer_handle buffer;

        /// Byte offset from the start of the buffer (used for dynamic or subrange bindings)
        uint32 offset = 0;

        /// Size of the bound range in bytes (0 means the entire buffer is bound)
        uint32 size = 0;

        /// Binding slot in the shader (matches `layout(binding=X)`)
        uint32 binding = 0;
    };

    /**
     * Describes a complete set of shader resource bindings
     * Includes textures/samplers and uniform/storage buffers
     */
    struct shader_binding_create_info
    {
        /// List of texture/sampler bindings
        core::static_vector<texture_binding, k_max_shader_textures> texture_bindings;

        /// List of buffer bindings
        core::static_vector<buffer_binding, k_max_shader_buffers> buffer_bindings;
    };

} // namespace tavros::renderer::rhi
