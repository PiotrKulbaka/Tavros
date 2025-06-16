#pragma once

#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer
{

    class command_list
    {
    public:
        virtual ~command_list() = default;

        virtual void bind_pipeline(pipeline_handle pipeline) = 0;

        virtual void bind_framebuffer(framebuffer_handle handle) = 0;

        virtual void bind_geometry(geometry_binding_handle geometry_binding) = 0;

        virtual void bind_texture(uint32 slot, texture_handle texture) = 0;
    };

} // namespace tavros::renderer
