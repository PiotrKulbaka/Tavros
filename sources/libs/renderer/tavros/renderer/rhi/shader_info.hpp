#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Specifies the stage of a shader program, such as vertex or fragment.
     */
    enum class shader_stage : uint8
    {
        vertex,   /// Vertex shader
        fragment, /// Fragment shader
    };

    /**
     * Represents information about a shader program
     */
    struct shader_info
    {
        // The source code of the shader to be compiled
        core::string_view source_code;

        // The stage of the shader (e.g., vertex, fragment, etc.)
        shader_stage stage = shader_stage::vertex;

        // The entry point function name in the shader code
        core::string_view entry_point = "main";
    };

} // namespace tavros::renderer::rhi
