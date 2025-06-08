#pragma once

#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer
{

    class command_list
    {
    public:
        virtual ~command_list() = default;

        virtual void bind_sampler(uint32 slot, sampler_handle sampler) = 0;
        virtual void bind_texture(uint32 slot, texture2d_handle texture) = 0;
        virtual void bind_pipeline(pipeline_handle pipeline) = 0;
    };

} // namespace tavros::renderer
