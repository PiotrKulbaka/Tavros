#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/containers/fixed_vector.hpp>
#include <tavros/assets/image/image.hpp>
#include <tavros/assets/image/image_view.hpp>

namespace tavros::renderer
{

    /**
     * @brief Utility class for generating CPU-side mipmap chains from a base image.
     *
     * All methods are static. The class is non-constructable.
     *
     * Mip level 0 is always the original base image and is NOT included
     * in the output of generate() - it is expected to be passed separately
     * to texture_uploader::upload_2d().
     */
    class mipmap_generator : core::nonconstructable
    {
    public:
        /** @brief Maximum supported mip levels. */
        static constexpr size_t k_max_mips = 32;

        /** @brief Container type for a mip chain (levels 1..N, excluding level 0). */
        using mipmap_levels = core::fixed_vector<assets::image, k_max_mips>;

        /**
         * @brief Generates a single mip level from a base image.
         *
         * The output dimensions are: max(1, base.width >> level) x max(1, base.height >> level).
         *
         * @param base  Source image to downsample. Must be valid.
         * @param level Mip level index (1 = half size, 2 = quarter size, etc.).
         * @param srgb  If true, performs gamma-correct downsampling (sRGB color space).
         *              Use true for color textures, false for linear data (normals, masks).
         * @return Downsampled image at the requested mip level.
         */
        [[nodiscard]] static assets::image generate_level(assets::image_view base, uint32 level, bool srgb = true);

        /**
         * @brief Generates a full mip chain from a base image.
         *
         * Mip level 0 (the original image) is NOT included in the result.
         * The returned vector contains levels 1..levels-1.
         *
         * @param base   Source image. Must be valid.
         * @param levels Number of mip levels to generate (including level 0).
         *               Pass 0 to generate the full chain down to 1x1.
         * @param srgb   If true, performs gamma-correct downsampling.
         * @return Mip chain containing levels 1..N (excluding level 0).
         */
        [[nodiscard]] static mipmap_levels generate(assets::image_view base, uint32 levels = 0, bool srgb = true);
    };

} // namespace tavros::renderer
