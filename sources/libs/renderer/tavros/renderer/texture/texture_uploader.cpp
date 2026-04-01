#include <tavros/renderer/texture/texture_uploader.hpp>

namespace tavros::renderer
{

    void texture_uploader::upload_2d_level(rhi::texture_handle gpu_tex, assets::image_view im, uint32 mip_level, uint32 layer_index, gpu_stage_buffer& stage, rhi::command_queue& cmd)
    {
        auto slice = stage.slice<uint8>(im.width() * im.height() * im.components());

        rhi::texture_copy_region region;
        region.mip_level = mip_level;
        region.layer_index = layer_index;
        region.width = im.width();
        region.height = im.height();
        region.depth = 1;
        region.buffer_offset = slice.offset_bytes();
        region.buffer_row_length = 0; // tightly packed

        auto   h = im.height();
        auto   row_sz = im.row_size_bytes();
        size_t offset = 0;
        auto   dst = slice.data();
        for (uint32 y = 0; y < h; ++y) {
            dst.copy_from(im.row(y), row_sz, offset);
            offset += row_sz;
        }

        cmd.copy_buffer_to_texture(stage.gpu_buffer(), gpu_tex, region);
    }

    void texture_uploader::upload_2d(rhi::texture_handle gpu_tex, assets::image_view base, core::buffer_view<assets::image> levels, uint32 layer_index, gpu_stage_buffer& stage, rhi::command_queue& cmd)
    {
        upload_2d_level(gpu_tex, base, 0, layer_index, stage, cmd);
        auto sz = static_cast<uint32>(levels.size());
        for (uint32 i = 0; i < sz; ++i) {
            upload_2d_level(gpu_tex, levels[i], i + 1, layer_index, stage, cmd);
        }
    }

} // namespace tavros::renderer
