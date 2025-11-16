#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/dynamic_buffer.hpp>
#include <tavros/core/containers/static_vector.hpp>

#include <tavros/text/font/font.hpp>

namespace tavros::text
{

    /**
     * @brief Font atlas builder and storage manager.
     *
     * This class collects multiple fonts and builds a single packed bitmap atlas
     * containing SDF or grayscale glyph images. After rebuilding, it updates UV
     * coordinates for all registered fonts so they reference the correct regions
     * inside the atlas.
     *
     * The atlas is stored inside a user-provided dynamic buffer. When rebuilding,
     * the buffer is resized if its capacity is insufficient. The atlas uses
     * single-channel (8-bit) pixel data.
     *
     * A rebuild is required whenever glyph sets change or when any font signals
     * modifications that affect its atlas placement.
     */
    class font_atlas : core::noncopyable
    {
    public:
        /**
         * @brief Describes a baked atlas bitmap stored inside an external buffer.
         *
         * The @p pixels pointer references memory owned by the caller-provided
         * dynamic buffer passed to invalidate_old_and_bake_new_atlas(). It remains
         * valid until that buffer is resized.
         */
        struct atlas_pixels
        {
            /// Pointer to 8-bit grayscale pixel data.
            uint8* pixels = nullptr;

            /// Atlas width in pixels.
            uint32 width = 0;

            /// Atlas height in pixels.
            uint32 height = 0;

            /// Number of bytes per row.
            uint32 stride = 0;
        };

    public:
        /**
         * @brief Constructs an empty atlas.
         */
        font_atlas();

        /**
         * @brief Destroys the atlas.
         */
        ~font_atlas();

        /**
         * @brief Registers a font to be included in the atlas.
         *
         * The atlas will include glyphs from all registered fonts. Calling this function
         * marks the atlas as needing a rebuild.
         *
         * @param fnt Font instance to register.
         */
        void register_font(font* fnt) noexcept;

        /**
         * @brief Checks whether the atlas must be rebuilt.
         *
         * This becomes true when a font is registered or when a previously registered
         * font reports changes that affect its glyph data or UV layout.
         *
         * @return True if atlas needs to be recreated.
         */
        bool need_to_recreate_atlas() const noexcept;

        /**
         * @brief Rebuilds the font atlas and writes its pixel data into @p storage.
         *
         * This function recreates the atlas for all registered fonts, updates UV coordinates
         * in these fonts, and writes the resulting single-channel (8-bit) grayscale/SDF bitmap
         * into the supplied @p storage buffer.
         *
         * If @p storage does not have enough capacity, it will be resized to the minimum
         * required size. Pixel data is written starting from the beginning of the buffer.
         *
         * The returned atlas_pixels structure contains metadata and a pointer to the pixel
         * data inside @p storage. The pointer remains valid until @p storage is resized again.
         *
         * @param storage          Dynamic buffer used as the memory for the atlas bitmap.
         *                         Will be resized if its capacity is insufficient.
         * @param glyph_scale_pix  Glyph size in pixels used during baking.
         * @param glyph_sdf_pad_pix  Amount of SDF padding around each glyph, in pixels.
         *
         * @return atlas_pixels     View describing the baked atlas data stored in @p storage.
         */
        atlas_pixels invalidate_old_and_bake_new_atlas(core::dynamic_buffer<uint8>& storage, float glyph_scale_pix, float glyph_sdf_pad_pix);

    private:
        /// Maximum number of fonts the atlas can store without dynamic allocation.
        /// A fixed-capacity container is used because typical usage rarely requires
        /// many fonts in a single atlas, and this avoids heap overhead entirely.
        /// If a project needs to support more fonts, the capacity may be increased
        /// or the container can be replaced with a dynamically sized core::vector.
        constexpr static size_t k_max_fonts = 128;

        core::static_vector<font*, k_max_fonts> m_fonts;

        bool m_need_to_recreate;
    };

} // namespace tavros::text
