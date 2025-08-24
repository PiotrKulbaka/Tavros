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

        void bind_geometry(geometry_binding_handle geometry_binding) override;

        void bind_texture(uint32 slot, texture_handle texture) override;

        void begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer) override;

        void end_render_pass() override;

    private:
        graphics_device_opengl* m_device = nullptr;
        uint32                  m_current_pipeline_id = 0;
        render_pass_handle      m_current_render_pass = {0};
        framebuffer_handle      m_current_framebuffer = {0};

        GLuint m_resolve_fbo = 0;
    };

} // namespace tavros::renderer
