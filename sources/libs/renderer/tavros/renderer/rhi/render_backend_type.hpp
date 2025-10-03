#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer::rhi
{

    enum class render_backend_type : uint8
    {
        opengl,
        vulkan,
        directx12,
        metal,
    };

} // namespace tavros::renderer::rhi
