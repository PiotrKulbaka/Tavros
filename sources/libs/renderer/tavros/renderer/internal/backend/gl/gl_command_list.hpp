#pragma once

#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/internal/backend/gl/gl_graphics_device.hpp>

namespace tavros::renderer
{

    class gl_command_list final : public command_list
    {
    public:
        gl_command_list(gl_graphics_device* device);
        ~gl_command_list() override;

        void bind_pipeline(pipeline_handle pipeline) override;

        void bind_framebuffer(framebuffer_handle handle) override;

        void bind_geometry(geometry_binding_handle geometry_binding) override;

    private:
        gl_graphics_device* m_device = nullptr;
        uint32              m_current_pipeline_id = 0;
    };

} // namespace tavros::renderer
