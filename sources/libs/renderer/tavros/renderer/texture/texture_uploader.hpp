#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/containers/fixed_vector.hpp>
#include <tavros/assets/image/image.hpp>
#include <tavros/assets/image/image_view.hpp>

#include <tavros/renderer/gpu_stage_buffer.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>

namespace tavros::renderer
{

    /**
     * @brief Utility class for uploading CPU image data to GPU textures via a staging buffer.
     *
     * All methods are static. The class is non-constructable.
     *
     * The GPU texture must already be created with the correct dimensions and mip level count.
     */
    class texture_uploader : core::nonconstructable
    {
    public:
        /**
         * @brief Uploads a single image to one mip level of a 2D GPU texture.
         *
         * @param gpu_tex   Target GPU texture handle. Must be valid.
         * @param im        Source image data. Dimensions must match the expected size for @p mip_level.
         * @param mip_level Destination mip level index (0 = full resolution).
         * @param stage     Staging buffer used for the CPU->GPU transfer.
         * @param cmd       Command queue to record the copy command into.
         */
        static void upload_2d_level(rhi::texture_handle gpu_tex, assets::image_view im, uint32 mip_level, uint32 layer_index, gpu_stage_buffer& stage, rhi::command_queue& cmd);

        /**
         * @brief Uploads a base image and its mip chain to a 2D GPU texture.
         *
         * Uploads @p base as mip level 0, then each entry in @p levels as
         * mip levels 1..N in order. The GPU texture must have been created
         * with at least 1 + levels.size() mip levels.
         *
         * @param gpu_tex  Target GPU texture handle. Must be valid.
         * @param base     Source image for mip level 0. Must be valid.
         * @param levels   Mip levels 1..N, as produced by mipmap_generator::generate().
         *                 May be empty if the texture has only one mip level.
         * @param stage    Staging buffer used for the CPU->GPU transfer.
         * @param cmd      Command queue to record the copy commands into.
         */
        static void upload_2d(rhi::texture_handle gpu_tex, assets::image_view base, core::buffer_view<assets::image> levels, uint32 layer_index, gpu_stage_buffer& stage, rhi::command_queue& cmd);
    };

} // namespace tavros::renderer
