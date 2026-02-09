#pragma once

#include <tavros/text/font/font.hpp>
#include <tavros/core/pimpl.hpp>
#include <tavros/core/containers/vector.hpp>

namespace tavros::text
{

    /**
     * @brief TrueType font implementation with SDF glyph baking support.
     *
     * This class loads and processes TrueType font data, extracts glyph metrics,
     * generates SDF glyph bitmaps, and provides kerning information.
     *
     * It extends the base @ref font interface and serves as a concrete backend
     * for TrueType-based fonts used by the engine.
     */
    class truetype_font final : public font
    {
    public:
        /**
         * @brief Represents a continuous inclusive Unicode range.
         *
         * Only glyphs whose codepoints fall within the provided ranges will be loaded.
         * Ranges must be sorted in ascending order and must not overlap.
         */
        struct codepoint_range
        {
            /// First Unicode codepoint in the range (inclusive).
            char32 first_codepoint = 0;

            /// Last Unicode codepoint in the range (inclusive).
            char32 last_codepoint = 0;
        };

    public:
        /**
         * @brief Constructs the font from raw TrueType data.
         *
         * The font data is expected to contain a full TTF or OTF file.
         * Only glyphs from the specified codepoint ranges are loaded.
         *
         * Ranges must be strictly increasing and must not overlap.
         * If initialization fails, the object remains in an uninitialized state.
         *
         * This method also constructs a special fallback null glyph (index 0),
         * which is always available for rendering.
         *
         * @param font_data         Raw font file stored in a dynamic buffer.
         * @param codepoint_ranges  List of Unicode ranges to load glyphs from.
         */
        truetype_font(core::vector<uint8> font_data, core::buffer_view<codepoint_range> codepoint_ranges);

        /**
         * @brief Destroys the font object.
         */
        ~truetype_font() noexcept override;

    protected:
        math::isize2 glyph_bitmap_size(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix) const noexcept override;

        void bake_glyph_bitmap(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix, core::buffer_span<uint8> pixels, uint32 pixels_stride) const noexcept override;

        float get_kerning_internal(char32 cp1, char32 cp2) const noexcept override;

    private:
        core::vector<uint8> m_font_data;
        float               m_scale;

        // Internal implementation details (stb_truetype state).
        struct impl;
        core::pimpl<impl, 160, 8> m_impl;
    };

} // namespace tavros::text
