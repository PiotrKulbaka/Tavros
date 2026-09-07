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
     * @brief Static utility for constructing and updating glyph-based text data.
     *
     * Builds and modifies an archetype containing:
     *  - glyph_c
     *  - atlas_rect_t
     *  - rect_layout_c
     *
     * Responsibilities:
     *  - UTF-8 decoding to Unicode codepoints
     *  - Glyph generation
     *  - Glyph metric resolution
     *  - Kerning application
     *  - Step advance computation
     *  - Atlas entry lookup
     *  - Layout rectangle calculation (including SDF padding)
     *
     * Coordinate system:
     *  - Baseline-oriented.
     *  - X axis increases to the right.
     *  - Y axis increases downwards.
     *  - glyph_c::ascent_y is negative.
     *  - glyph_c::descent_y is positive.
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
         * @brief Append UTF-8 text to existing glyph data.
         *
         * Decodes the string and appends glyph components with unresolved metrics.
         * Call resolve_metrics() for the appended range afterwards.
         *
         * @tparam Text      Archetype containing glyph_c.
         * @param  text      Target text container.
         * @param  str       UTF-8 encoded string.
         * @param  fnt       Font assigned to new glyphs (must not be null).
         * @param  font_size Font size scaling factor.
         * @return Number of appended glyphs.
         */
        template<core::archetype_with<glyph_c, atlas_rect_t, rect_layout_c> Text>
        static size_t append_text(Text& text, core::string_view str, const font* fnt, float font_size)
        {
            TAV_ASSERT(fnt != nullptr);

            auto codepoints = convert_to_codepoints(str);
            text.reserve(text.size() + codepoints.size());
            for (const auto cp : codepoints) {
                const auto g = glyph_c{
                    .font = fnt,
                    .codepoint = cp,
                    .size = font_size,
                    .step_x = 0.0f,   // Will be calculated in resolve_metrics
                    .ascent_y = 0.0f, // Will be calculated in resolve_metrics
                    .descent_y = 0.0f // Will be calculated in resolve_metrics
                };

                text.typed_emplace_back(glyph_c{fnt, cp, font_size, 0.0f, 0.0f, 0.0f});
            }
            return codepoints.size();
        }

        /**
         * @brief Change font and size for a glyph range.
         *
         * Updates glyph style and invalidates resolved metrics.
         * Call resolve_metrics() for the modified range afterwards.
         *
         * @tparam Text      Archetype containing glyph_c.
         * @param  text      Target text container.
         * @param  first     Index of the first glyph to modify.
         * @param  count     Number of glyphs to update.
         * @param  fnt       New font (must not be null).
         * @param  font_size New font size scaling factor.
         */
        template<core::archetype_with<glyph_c> Text>
        static void set_style(Text& text, size_t first, size_t count, const font* fnt, float font_size)
        {
            TAV_ASSERT(fnt != nullptr);
            TAV_ASSERT(first <= text.size());
            TAV_ASSERT(first + count <= text.size());

            if (count == 0) {
                return;
            }

            text.view<glyph_c>().each_n(first, count, [&](glyph_c& g) {
                g.font = fnt;
                // g.codepoint; // Keep the same codepoint
                g.size = font_size;
                g.step_x = 0.0f;    // Will be recalculated in resolve_metrics
                g.ascent_y = 0.0f;  // Will be recalculated in resolve_metrics
                g.descent_y = 0.0f; // Will be recalculated in resolve_metrics
            });
        }

        /**
         * @brief Resolve glyph metrics for a range.
         *
         * Computes glyph metrics, kerning, atlas entries and layout rectangles
         * for the specified range. Adjacent kerning pairs are updated when needed.
         *
         * @tparam Text  Archetype containing glyph_c, atlas_rect_t and rect_layout_c.
         * @param  text  Target text container.
         * @param  first Index of the first glyph to resolve.
         * @param  count Number of glyphs to resolve.
         */
        template<core::archetype_with<glyph_c, atlas_rect_t, rect_layout_c> Text>
        static void resolve_metrics(Text& text, size_t first, size_t count) noexcept
        {
            TAV_ASSERT(first <= text.size());
            TAV_ASSERT(first + count <= text.size());

            if (count == 0) {
                return;
            }

            auto&  glyphs = text.get<glyph_c>();
            size_t end = first + count;

            // Update previous glyph's step_x if the new font is the same as the previous one, otherwise reset it to the default advance
            if (first > 0) {
                glyph_c&    prev_g = glyphs[first - 1];
                glyph_c&    cur_g = glyphs[first];
                const auto& gi = prev_g.font->get_glyph_info(prev_g.font->find_glyph(prev_g.codepoint));

                auto step_x = gi.metrics.advance.x;
                if (prev_g.font->equals(*cur_g.font)) {
                    step_x += cur_g.font->get_kerning(prev_g.codepoint, glyphs[first].codepoint);
                }
                prev_g.step_x = step_x * prev_g.size;
            }

            text.view<glyph_c, atlas_rect_t, rect_layout_c>()
                .each_n_indexed(first, count, [&](size_t i, glyph_c& g, atlas_rect_t& r, rect_layout_c& l) {
                    const auto& gi = g.font->get_glyph_info(g.font->find_glyph(g.codepoint));

                    auto step_x = gi.metrics.advance.x;
                    // Apply kerning with the next glyph if it exists and uses the same font
                    if (i + 1 < end || (end < text.size() && g.font->equals(*glyphs[i].font))) {
                        step_x += g.font->get_kerning(g.codepoint, glyphs[i + 1].codepoint);
                    }

                    const auto& fm = g.font->get_font_metrics();
                    auto        pad = fm.sdf_padding_pix * g.size;

                    g.step_x = step_x * g.size;
                    g.ascent_y = -fm.ascent_y * g.size;
                    g.descent_y = -fm.descent_y * g.size;
                    g.sdf_pad_pix = pad;

                    // Update atlas rectangle for the glyph, or set to zero if it's a whitespace character
                    r = core::is_unicode_whitespace(g.codepoint) ? atlas_rect_t{0, 0, 0, 0} : gi.entry;

                    // Update layout rectangle with a font size and padding
                    auto b = gi.metrics.bearing * g.size;
                    auto s = gi.metrics.size * g.size;

                    l.value.min.x = b.x - pad;
                    l.value.min.y = b.y - s.y - pad;
                    l.value.max.x = b.x + s.x + pad;
                    l.value.max.y = b.y + pad;
                });
        }
    };

} // namespace tavros::renderer