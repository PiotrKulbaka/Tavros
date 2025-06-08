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

        void bind_sampler(uint32 slot, sampler_handle sampler) override;
        void bind_texture(uint32 slot, texture2d_handle texture) override;
        void bind_pipeline(pipeline_handle pipeline) override;

    private:
        gl_graphics_device* m_device = nullptr;
        uint32              m_current_pipeline_id = 0;
    };

} // namespace tavros::renderer
