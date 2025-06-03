#pragma once

#include <tavros/renderer/interfaces/renderer.hpp>

#include <tavros/renderer/texture2d_desc.hpp>


namespace tavros::renderer::interfaces
{

    class renderer
    {
    public:
        virtual ~renderer() = default;

        virtual begin_render_pass(render_pass_handle render_pass) = 0;

        virtual void end_render_pass() = 0;

        virtual graphics_device& graphics_device() = 0;
    };

} // namespace tavros::renderer
