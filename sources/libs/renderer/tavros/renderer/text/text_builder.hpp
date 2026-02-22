#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/utf8.hpp>
#include <tavros/core/archetype.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/renderer/text/glyph_data.hpp>
#include <tavros/renderer/text/line_data.hpp>
#include <tavros/renderer/components/all.hpp>

#include <cwctype>

namespace tavros::renderer
{

    /**
     * @brief Static utility for constructing and updating glyph-based text layout.
     *
     * Builds and modifies an archetype containing:
     *  - glyph_c
     *  - atlas_rect_t
     *  - rect_layout_c
     *
     * Responsibilities:
     *  - UTF-8 decoding to Unicode codepoints
     *  - Glyph lookup and metric extraction
     *  - Kerning application
     *  - Step advance computation
     *  - Layout rectangle calculation (including SDF padding)
     *
     * Coordinate system:
     *  - Baseline-oriented.
     *  - X axis increases to the right.
     *  - Y axis increases downwards.
     *  - glyph_c::ascent_y is negative.
     *  - glyph_c::descent_y is positive.
     *
     * Kerning guarantees:
     *  - step_x always represents advance to the next glyph.
     *  - When appending or restyling, adjacent kerning pairs are recalculated
     *    if both glyphs use the same font.
     *
     * The class is non-instantiable and contains only static methods.
     */
    class text_builder : core::nonconstructable
    {
    private:
        /**
         * @brief Decode a UTF-8 string into Unicode codepoints.
         *
         * @param str UTF-8 encoded string.
         * @return Vector of decoded Unicode codepoints.
         */
        static core::vector<char32> convert_to_codepoints(core::string_view str)
        {
            core::vector<char32> codepoints;
            codepoints.reserve(str.length());

            const char* it = str.data();
            const char* end = str.data() + str.length();

            char32 cp = 0;
            while (0 != (cp = core::extract_utf8_codepoint(it, end, &it))) {
                codepoints.push_back(cp);
            }

            return codepoints;
        }

    public:
        /**
         * @brief Replace text content with a new string.
         *
         * Clears existing glyph data and appends decoded glyphs
         * using the specified font and size.
         *
         * @tparam Text Archetype containing glyph_c, atlas_rect_t and rect_layout_c.
         * @param text Target text container.
         * @param str UTF-8 encoded string.
         * @param fnt Font used for glyph generation (must not be null).
         * @param font_size Font size scaling factor.
         */
        template<core::ArchetypeWith<glyph_c, atlas_rect_t, rect_layout_c> Text>
        static void set_text(Text& text, core::string_view str, const font* fnt, float font_size)
        {
            text.clear();
            append_text(text, str, fnt, font_size);
        }

        /**
         * @brief Append UTF-8 text to existing glyph data.
         *
         * Decodes the string, generates glyph components and appends them
         * to the container. Kerning with the previously existing last glyph
         * is updated when fonts match.
         *
         * Layout rectangles are computed from glyph metrics and SDF padding.
         *
         * @tparam Text Archetype containing glyph_c, atlas_rect_t and rect_layout_c.
         * @param text Target text container.
         * @param str UTF-8 encoded string.
         * @param fnt Font used for glyph generation (must not be null).
         * @param font_size Font size scaling factor.
         */
        template<core::ArchetypeWith<glyph_c, atlas_rect_t, rect_layout_c> Text>
        static void append_text(Text& text, core::string_view str, const font* fnt, float font_size)
        {
            TAV_ASSERT(fnt != nullptr);

            auto codepoints = convert_to_codepoints(str);

            text.reserve(text.size() + codepoints.size());

            auto cp_size = codepoints.size();
            if (cp_size > 0) {
                // Update step_x of the last glyph if the new text is appended to existing text and uses the same font.
                if (text.size() > 0) {
                    glyph_c&    last_g = text.get<glyph_c>().back();
                    const auto& gi = last_g.font->get_glyph_info(last_g.font->find_glyph(last_g.codepoint));

                    auto step_x = gi.metrics.advance.x;
                    if (last_g.font->equals(*fnt)) {
                        step_x += fnt->get_kerning(last_g.codepoint, codepoints[0]);
                    }
                    last_g.step_x = step_x * last_g.size;
                }

                const auto& fm = fnt->get_font_metrics();
                auto        pad = fm.sdf_padding_pix * font_size;

                for (size_t i = 0; i < cp_size; ++i) {
                    auto        cp = codepoints[i];
                    const auto& gi = fnt->get_glyph_info(fnt->find_glyph(cp));

                    auto step_x = gi.metrics.advance.x;
                    if (i + 1 < cp_size) {
                        step_x += fnt->get_kerning(cp, codepoints[i + 1]);
                    }

                    auto b = gi.metrics.bearing * font_size;
                    auto s = gi.metrics.size * font_size;

                    atlas_rect_t entry = std::iswspace(cp) ? atlas_rect_t{0, 0, 0, 0} : gi.entry;

                    text.emplace_back(
                        // Invert ascent and descent to match the UI coordinate system where Y increases downwards
                        glyph_c{fnt, cp, font_size, step_x * font_size, -fm.ascent_y * font_size, -fm.descent_y * font_size},
                        entry,
                        // Compute layout rectangle with padding
                        rect_layout_c{geometry::aabb2{b.x - pad, b.y - s.y - pad, b.x + s.x + pad, b.y + pad}}
                    );
                }
            }
        }

        /**
         * @brief Change font and size for a glyph range.
         *
         * Recomputes glyph metrics, kerning, atlas entries and layout rectangles
         * for the specified range. Also updates adjacent kerning where required.
         *
         * @tparam Text Archetype containing glyph_c, atlas_rect_t and rect_layout_c.
         * @param text Target text container.
         * @param first Index of the first glyph to modify.
         * @param count Number of glyphs to update.
         * @param fnt New font (must not be null).
         * @param font_size New font size scaling factor.
         */
        template<core::ArchetypeWith<glyph_c, atlas_rect_t, rect_layout_c> Text>
        static void set_style(Text& text, size_t first, size_t count, const font* fnt, float font_size)
        {
            TAV_ASSERT(fnt != nullptr);
            TAV_ASSERT(first <= text.size());
            TAV_ASSERT(first + count <= text.size());

            if (count == 0) {
                return;
            }

            auto&       glyphs = text.get<glyph_c>();
            size_t      end = first + count;
            const auto& fm = fnt->get_font_metrics();
            auto        pad = fm.sdf_padding_pix * font_size;

            // Update previous glyph's step_x if the new font is the same as the previous one, otherwise reset it to the default advance
            if (first > 0) {
                glyph_c&    prev_g = glyphs[first - 1];
                const auto& gi = prev_g.font->get_glyph_info(prev_g.font->find_glyph(prev_g.codepoint));

                auto step_x = gi.metrics.advance.x;
                if (prev_g.font->equals(*fnt)) {
                    step_x += fnt->get_kerning(prev_g.codepoint, glyphs[first].codepoint);
                }
                prev_g.step_x = step_x * prev_g.size;
            }

            text.view<glyph_c, atlas_rect_t, rect_layout_c>()
                .each_n_indexed(first, count, [&](size_t i, glyph_c& g, atlas_rect_t& r, rect_layout_c& l) {
                    const auto& gi = fnt->get_glyph_info(fnt->find_glyph(g.codepoint));

                    auto step_x = gi.metrics.advance.x;
                    // Apply kerning with the next glyph if it exists and uses the same font
                    if (i + 1 < end || (end < text.size() && fnt->equals(*glyphs[i].font))) {
                        step_x += fnt->get_kerning(g.codepoint, glyphs[i + 1].codepoint);
                    }

                    g.font = fnt;
                    g.size = font_size;
                    g.step_x = step_x * font_size;
                    // Invert ascent and descent to match the UI coordinate system where Y increases downwards
                    g.ascent_y = -fm.ascent_y * font_size;
                    g.descent_y = -fm.descent_y * font_size;

                    r = std::iswspace(g.codepoint) ? atlas_rect_t{0, 0, 0, 0} : gi.entry;

                    // Update layout rectangle with new font size and padding
                    auto b = gi.metrics.bearing * font_size;
                    auto s = gi.metrics.size * font_size;

                    l.min.x = b.x - pad;
                    l.min.y = b.y - s.y - pad;
                    l.max.x = b.x + s.x + pad;
                    l.max.y = b.y + pad;
                });
        }
    };

} // namespace tavros::renderer