#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    template<typename ResourceTag>
    struct handle_base
    {
        uint32 id = 0;
    };

    struct swapchain_tag
    {
    };
    struct sampler_tag
    {
    };
    struct texture_tag
    {
    };
    struct pipeline_tag
    {
    };
    struct framebuffer_tag
    {
    };
    struct buffer_tag
    {
    };
    struct geometry_binding_tag
    {
    };

    using swapchain_handle = handle_base<swapchain_tag>;
    using sampler_handle = handle_base<sampler_tag>;
    using texture_handle = handle_base<texture_tag>;
    using pipeline_handle = handle_base<pipeline_tag>;
    using framebuffer_handle = handle_base<framebuffer_tag>;
    using buffer_handle = handle_base<buffer_tag>;
    using geometry_binding_handle = handle_base<geometry_binding_tag>;

} // namespace tavros::renderer
