#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/math/ivec2.hpp>
#include <tavros/core/math/vec4.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/containers/vector.hpp>

#include <tavros/text/font/atlas_rect.hpp>

namespace tavros::text
{

    class font_atlas;

    /**
     * @brief Represents a font and provides access to glyph metrics, atlas data, and kerning.
     *
     * This class exposes glyph information and font-wide metrics needed for text layout.
     * It also defines an interface for generating SDF bitmaps for individual glyphs.
     * The implementation is font-backend specific (e.g., TrueType).
     */
    class font : core::noncopyable
    {
    public:
        using glyph_index = uint32;

        /**
         * @brief Per-glyph metric information.
         *
         * These values describe how a glyph should be placed relative to the text cursor.
         */
        struct glyph_metrics
        {
            /// Advance vector applied to the text cursor after rendering the glyph
            math::vec2 advance;

            /// Offset from the cursor position to the top-left corner of the glyph
            math::vec2 bearing;

            /// Normalized glyph width and height
            math::size2 size;
        };

        /**
         * @brief Combined glyph information.
         *
         * Stores the codepoint, atlas data, and metrics needed for rendering and layout.
         */
        struct glyph_info
        {
            /// Unicode codepoint
            char32 codepoint = 0;

            /// Atlas entry describing UV placement.
            atlas_rect entry;

            /// Layout and rendering metrics.
            glyph_metrics metrics;
        };

        /**
         * @brief Font-wide metric values.
         *
         * These values describe the vertical layout characteristics of the font.
         * Common sign convention:
         *   - ascent_y  > 0 (above baseline)
         *   - descent_y < 0 (below baseline)
         *   - line_gap_y > 0 (extra spacing between lines)
         */
        struct font_metrics
        {
            /// Distance from the baseline to the highest ascender. Usually positive.
            float ascent_y = 0.0f;

            /// Distance from the baseline to the lowest descender. Usually negative.
            float descent_y = 0.0f;

            /// Additional space between lines. Usually positive.
            float line_gap_y = 0.0f;

            /// Additional padding in pixels used when generating SDF bitmaps.
            float sdf_padding_pix = 0.0f;
        };

    public:
        /**
         * @brief Destroys the font instance.
         */
        virtual ~font() noexcept = default;

        bool equals(const font& other) const noexcept;

        /**
         * @brief Finds the glyph index for a given Unicode codepoint.
         *
         * If the requested codepoint is not present in the font, the function returns 0.
         * Index 0 always corresponds to a special fallback "null glyph" that is guaranteed
         * to exist and can be rendered. This ensures that missing characters can still be
         * visualized using the fallback glyph.
         *
         * @param codepoint Unicode character to search for.
         * @return Glyph index, or 0 if the glyph is not present.
         */
        glyph_index find_glyph(char32 codepoint) const noexcept;

        /**
         * @brief Returns font-wide vertical metrics.
         */
        const font_metrics& get_font_metrics() const noexcept;

        /**
         * @brief Returns glyph data for the given glyph index.
         *
         * @param idx Index of the glyph in the internal list.
         */
        const glyph_info& get_glyph_info(glyph_index idx) const noexcept;

        /**
         * @brief Returns kerning adjustment between two glyphs.
         *
         * @param left_codepoint  left glyph codepoint.
         * @param right_codepoint right glyph codepoint.
         * @return Horizontal kerning offset.
         */
        float get_kerning(char32 left_codepoint, char32 right_codepoint) const noexcept;

    protected:
        /**
         * @brief Computes the pixel size of a glyph's bitmap.
         *
         * @param idx              Glyph index.
         * @param glyph_scale_pix  Pixel scale factor.
         * @param glyph_sdf_pad_pix Additional SDF padding, in pixels.
         * @return Required size of the glyph bitmap.
         */
        virtual math::isize2 glyph_bitmap_size(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix) const noexcept = 0;

        /**
         * @brief Generates a glyph bitmap (typically SDF) into a provided buffer.
         *
         * @param idx              Glyph index.
         * @param glyph_scale_pix  Pixel scale factor.
         * @param glyph_sdf_pad_pix Additional SDF padding, in pixels.
         * @param pixels           Destination pixel buffer.
         * @param pixels_stride    Row stride of the destination buffer.
         */
        virtual void bake_glyph_bitmap(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix, core::buffer_span<uint8> pixels, uint32 pixels_stride) const noexcept = 0;

        /**
         * @brief Internal method for retrieving kerning between two codepoints.
         *
         * Backend implementations must override this to provide kerning information.
         */
        virtual float get_kerning_internal(char32 cp1, char32 cp2) const noexcept = 0;

    protected:
        friend font_atlas;

        font_metrics             m_font_metrics;
        core::vector<glyph_info> m_glyphs; /// Sorted by glyph_info::codepoint
    };

} // namespace tavros::text
