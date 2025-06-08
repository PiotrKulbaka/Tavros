#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/internal/backend/gl/gl_pipeline.hpp>
#include <tavros/core/containers/unordered_map.hpp>

namespace tavros::renderer
{
    class gl_command_list;

    class gl_graphics_device final : public tavros::renderer::graphics_device
    {
    public:
        gl_graphics_device();
        ~gl_graphics_device() override;

        sampler_handle create_sampler(const sampler_desc& desc) override;
        void           destroy_sampler(sampler_handle handle) override;

        texture2d_handle create_texture(const texture2d_desc& desc) override;
        void             destroy_texture(texture2d_handle handle) override;

        pipeline_handle create_pipeline(const pipeline_desc& desc) override;
        void            destroy_pipeline(pipeline_handle pipeline) override;

    private:
        core::unordered_map<uint32, sampler_desc> m_textures;
        core::unordered_map<uint32, gl_pipeline>  m_pipelines;

        friend gl_command_list;
    };

} // namespace tavros::renderer
