#pragma once

#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

namespace tavros::renderer
{

    class command_list_opengl final : public command_list
    {
    public:
        command_list_opengl(graphics_device_opengl* device);
        ~command_list_opengl() override;

        void bind_pipeline(pipeline_handle pipeline) override;

        void bind_framebuffer(framebuffer_handle handle) override;

        void bind_geometry(geometry_binding_handle geometry_binding) override;

    private:
        graphics_device_opengl* m_device = nullptr;
        uint32                  m_current_pipeline_id = 0;
    };

} // namespace tavros::renderer
