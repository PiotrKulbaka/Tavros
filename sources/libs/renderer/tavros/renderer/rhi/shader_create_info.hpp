#pragma once

#include <tavros/renderer/rhi/enums.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Represents information about a shader program
     */
    struct shader_create_info
    {
        // The source code of the shader to be compiled
        core::string_view source_code;

        // The stage of the shader (e.g., vertex, fragment, etc.)
        shader_stage stage = shader_stage::vertex;

        // The entry point function name in the shader code
        core::string_view entry_point = "main";
    };

} // namespace tavros::renderer::rhi
