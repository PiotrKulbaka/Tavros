#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/type_storage.hpp>
#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/renderer/ui_renderer.hpp>

namespace tavros::renderer
{
    // fdt - frame data type

    struct fdt_frame_info
    {
        math::vec2 frame_size;
        float      total_time = 0.0f;
        float      delta_time = 0.0f;
    };

    struct fdt_camera
    {
        camera cam;
    };

    struct fdt_backbuffer
    {
        rhi::framebuffer_handle fb;
    };

    struct fdt_albedo_ca
    {
        rhi::texture_handle tex;
    };

    struct fdt_ui_draw_data
    {
        ui_render_data_t data;
    };


    using frame_types = core::type_list<
        fdt_frame_info,
        fdt_camera,
        fdt_backbuffer,
        fdt_albedo_ca,
        fdt_ui_draw_data>;

    using frame_data_t = core::type_storage<frame_types, 256>;
} // namespace tavros::renderer
