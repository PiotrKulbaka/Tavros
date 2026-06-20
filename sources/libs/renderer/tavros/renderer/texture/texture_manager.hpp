#pragma once

#include <tavros/core/resource/resource_manager.hpp>
#include <tavros/assets/image/image_view.hpp>
#include <tavros/renderer/texture/texture.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/renderer/gpu_stage_buffer.hpp>

namespace tavros::renderer
{

    class texture_manager : public core::basic_resource_manager<texture_t>
    {
    public:
        /**
         * @brief Parameters controlling how a source image is interpreted
         *        and what GPU object is created from it.
         */
        struct load_params
        {
            /// X offset into the source image in pixels
            uint32 left = 0;

            /// Y offset into the source image in pixels
            uint32 top = 0;

            /// Width of the source region in pixels (if 0 -> im.width() - left)
            uint32 width = 0;

            /// Height of the source region in pixels (if 0 -> im.height() - top)
            uint32 height = 0;

            /// Depth of the source region in pixels (texture_3d only)
            uint32 depth = 1;

            /// Whether to generate a full mipmap chain
            bool gen_mipmaps = true;

            /// Type of the GPU texture to create
            rhi::texture_type type = rhi::texture_type::texture_2d;

            /// Target pixel format (if none -> inferred from source image)
            rhi::pixel_format pixel_format = rhi::pixel_format::none;

            /// Number of array layers to create (if 1 -> regular texture, if > 1 -> texture array)
            uint32 array_layers = 1;

            /// Number of tile rows in the source image grid (0 = auto-detected)
            uint32 array_rows = 0;

            /// Number of tile columns in the source image grid (0 = auto-detected)
            uint32 array_cols = 0; // For texture arrays, number of columns in the source image (0 - detect automatically)
        };

    public:
        texture_manager(rhi::graphics_device* gdevice) noexcept
            : m_gdevice(gdevice)
        {
        }

        ~texture_manager() noexcept = default;

        void release_resource(texture_t& res) noexcept override
        {
            m_gdevice->safe_destroy(res.gpu_texture);
        }

        resource_ref_type load(
            gpu_stage_buffer& stage,
            rhi::command_queue& cmd,
            core::string_view path,
            const load_params& params = {}
        );

        resource_ref_type load(
            gpu_stage_buffer& stage,
            rhi::command_queue& cmd,
            assets::image_view  im,
            core::string_view   key,
            const load_params& params = {}
        );

    private:
        rhi::graphics_device* m_gdevice = nullptr;
    };

}
