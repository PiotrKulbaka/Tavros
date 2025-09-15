#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/core/containers/sapn.hpp>
#include <tavros/renderer/rhi/limits.hpp>

namespace tavros::renderer
{

    /**
     * Describes a texture + sampler binding to a shader stage
     */
    struct texture_binding
    {
        /// Index into the texture handle array passed to create_shader_binding
        uint32 texture_index = 0;

        /// Index into the sampler handle array passed to create_shader_binding
        uint32 sampler_index = 0;

        /// Binding slot in the shader (matches `layout(binding=X)` in GLSL/HLSL)
        uint32 binding = 0;
    };

    /**
     * Describes a buffer or subbuffer binding to a shader stage
     */
    struct buffer_binding
    {
        /// Index into the buffer handle array passed to create_shader_binding
        uint32 buffer_index = 0;

        /// Byte offset into the buffer (for dynamic offsets)
        uint32 offset = 0;

        /// Size in bytes (0 means bind the entire buffer)
        uint32 size = 0;

        /// Binding slot in the shader (matches `layout(binding=X)`)
        uint32 binding = 0;
    };

    /**
     * Describes a complete set of shader resource bindings
     * Includes textures/samplers and uniform/storage buffers
     */
    struct shader_binding_desc
    {
        /// List of texture/sampler bindings
        core::static_vector<texture_binding, k_max_shader_textures> texture_bindings;

        /// List of buffer bindings
        core::static_vector<buffer_binding, k_max_shader_buffers> buffer_bindings;
    };

} // namespace tavros::renderer
