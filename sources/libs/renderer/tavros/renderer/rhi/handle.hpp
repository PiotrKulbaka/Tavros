#pragma once

#include <tavros/core/ids/handle_base.hpp>

namespace tavros::renderer::rhi
{

    struct frame_composer_tag : core::handle_type_registration<0x1001>
    {
    };
    struct sampler_tag : core::handle_type_registration<0x1002>
    {
    };
    struct texture_tag : core::handle_type_registration<0x1003>
    {
    };
    struct pipeline_tag : core::handle_type_registration<0x1004>
    {
    };
    struct framebuffer_tag : core::handle_type_registration<0x1005>
    {
    };
    struct buffer_tag : core::handle_type_registration<0x1006>
    {
    };
    struct render_pass_tag : core::handle_type_registration<0x1007>
    {
    };
    struct shader_tag : core::handle_type_registration<0x1009>
    {
    };
    struct fence_tag : core::handle_type_registration<0x100a>
    {
    };

    using frame_composer_handle = core::handle_base<frame_composer_tag>;
    using sampler_handle = core::handle_base<sampler_tag>;
    using texture_handle = core::handle_base<texture_tag>;
    using pipeline_handle = core::handle_base<pipeline_tag>;
    using framebuffer_handle = core::handle_base<framebuffer_tag>;
    using buffer_handle = core::handle_base<buffer_tag>;
    using render_pass_handle = core::handle_base<render_pass_tag>;
    using shader_handle = core::handle_base<shader_tag>;
    using fence_handle = core::handle_base<fence_tag>;

} // namespace tavros::renderer::rhi
