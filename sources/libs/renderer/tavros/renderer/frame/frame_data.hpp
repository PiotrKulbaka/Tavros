#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/type_storage.hpp>
#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/renderer/ui_renderer.hpp>
#include <tavros/renderer/renderer/debug_renderer.hpp>

namespace tavros::renderer
{
    // fd - frame data type

    struct fd_frame_info
    {
        math::vec2 frame_size;
        float      total_time = 0.0f;
        float      delta_time = 0.0f;
    };

    struct fd_graphics_settings
    {
        uint32 msaa = 1;
    };

    struct fd_camera
    {
        camera cam;
    };

    struct fd_backbuffer
    {
        rhi::framebuffer_handle fb;
    };

    struct fd_base_color_ca
    {
        rhi::texture_handle tex;
    };

    struct fd_depth_stencil_ca
    {
        rhi::texture_handle tex;
    };

    struct fd_ui_draw_data
    {
        ui_render_data_t data;
    };

    struct fd_debug_draw_data
    {
        debug_render_data_t data;
    };

    using frame_types = core::type_list<
        fd_frame_info,
        fd_camera,
        fd_backbuffer,
        fd_base_color_ca,
        fd_depth_stencil_ca,
        fd_ui_draw_data,
        fd_graphics_settings,
        fd_debug_draw_data>;

    using frame_data_t = core::type_storage<frame_types>;
} // namespace tavros::renderer
