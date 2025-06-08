#pragma once

#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/sampler_desc.hpp>
#include <tavros/renderer/rhi/texture2d_desc.hpp>
#include <tavros/renderer/rhi/pipeline_desc.hpp>

namespace tavros::renderer
{

    class graphics_device
    {
    public:
        virtual ~graphics_device() = default;

        virtual sampler_handle create_sampler(const sampler_desc& desc) = 0;
        virtual void           destroy_sampler(sampler_handle sampler) = 0;

        virtual texture2d_handle create_texture(const texture2d_desc& sampler) = 0;
        virtual void             destroy_texture(texture2d_handle texture) = 0;

        virtual pipeline_handle create_pipeline(const pipeline_desc& desc) = 0;
        virtual void            destroy_pipeline(pipeline_handle pipeline) = 0;
    };

} // namespace tavros::renderer

