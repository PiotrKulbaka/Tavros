#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/archetype.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/renderer/components/all.hpp>
#include <tavros/renderer/text/glyph_data.hpp>
#include <tavros/renderer/text/line_data.hpp>

#include <cwctype>

namespace tavros::renderer
{

    /**
     * @brief Sequential line breaking utility for glyph streams.
     *
     * Splits a linear sequence of glyph_c into logical lines and computes
     * per-line metrics:
     *  - glyph range
     *  - maximal ascent
     *  - maximal descent
     *  - visual width (excluding trailing whitespace)
     *
     * Behavior:
     *  - Processes glyphs in logical order.
     *  - Stops at explicit newline (U'\n').
     *  - Optionally performs word wrapping when enabled.
     *  - Trailing whitespace does not contribute to line width.
     *
     * Word wrapping model:
     *  - If wrap_words is false, lines only break on newline.
     *  - If wrap_words is true and max_line_width > 0:
     *      * Break occurs at the last whitespace before overflow.
     *      * If the first word exceeds max_line_width,
     *        it is placed on its own line without splitting.
     *
     * Vertical metrics:
     *  - ascent_y = minimal (most negative) ascent in the line.
     *  - descent_y = maximal descent in the line.
     *  - Metrics are propagated from glyph_c without modification.
     *
     * Non-instantiable. All functionality is static.
     */
    class text_line_breaker : core::nonconstructable
    {
    public:
        /**
         * @brief Determine the next line boundary.
         *
         * Processes glyphs starting at index i and returns the index
         * of the first glyph belonging to the next line.
         *
         * Outputs computed line metrics via reference parameters.
         *
         * Preconditions:
         *  - i < end
         *  - glyphs size >= end
         *
         * @param i Starting glyph index.
         * @param end One-past-last glyph index.
         * @param glyphs Glyph buffer.
         * @param ascent_y Output: maximal ascent of the line.
         * @param descent_y Output: maximal descent of the line.
         * @param width Output: visual line width excluding trailing whitespace.
         * @param wrap_words Enable word wrapping.
         * @param max_line_width Maximum allowed width when wrapping is enabled.
         *
         * @return Index of the first glyph of the next line.
         */
        static uint32 break_next_line(
            uint32 i, uint32 end,
            core::buffer_view<glyph_c> glyphs,
            float& ascent_y, float& descent_y, float& width,
            bool wrap_words = false, float max_line_width = 0.0f
        ) noexcept
        {
            TAV_ASSERT(i < end);

            float actual_width = width = glyphs[i].step_x;
            float actual_ascent_y = ascent_y = glyphs[i].ascent_y;
            float actual_descent_y = descent_y = glyphs[i].descent_y;
            bool  in_word_seq = !std::iswspace(glyphs[i].codepoint);
            bool  is_first_word = in_word_seq;

            if (U'\n' == glyphs[i].codepoint) {
                return i + 1;
            }

            uint32 best_seq = ++i;
            while (i < end) {
                const glyph_c& g = glyphs[i++];

                actual_width += g.step_x;
                if (actual_ascent_y > g.ascent_y) {
                    actual_ascent_y = g.ascent_y;
                }
                if (actual_descent_y < g.descent_y) {
                    actual_descent_y = g.descent_y;
                }

                if (std::iswspace(g.codepoint)) {
                    if (in_word_seq) {
                        width = actual_width - g.step_x;
                        in_word_seq = false;
                    }
                    ascent_y = actual_ascent_y;
                    descent_y = actual_descent_y;
                    best_seq = i;

                    is_first_word = false;
                    if (U'\n' == g.codepoint) {
                        return i;
                    }
                } else if (wrap_words && actual_width > max_line_width) {
                    return best_seq;
                } else {
                    if (is_first_word) {
                        width = actual_width;
                        ascent_y = actual_ascent_y;
                        descent_y = actual_descent_y;
                        best_seq = i;
                    }
                    in_word_seq = true;
                }
            }

            width = actual_width;
            ascent_y = actual_ascent_y;
            descent_y = actual_descent_y;
            return end;
        }

        /**
         * @brief Break entire glyph sequence into lines.
         *
         * Clears the output container and fills it with text_line_c entries.
         *
         * Wrapping is enabled when line_wrap_width > 0.
         *
         * Guarantees:
         *  - lines covers the entire glyph range without gaps.
         *  - Each glyph belongs to exactly one line.
         *  - Lines preserve glyph order.
         *
         * @tparam Text Archetype containing glyph_c.
         * @tparam Lines Archetype containing text_line_c.
         *
         * @param text Glyph container.
         * @param lines Output line container.
         * @param line_wrap_width Maximum allowed width (0 disables wrapping).
         */
        template<
            core::ArchetypeWith<glyph_c>     Text,
            core::ArchetypeWith<text_line_c> Lines>
        static void break_lines(Text& text, Lines& lines, float line_wrap_width = 0.0f)
        {
            lines.clear();

            // Iterate until all glyphs are processed
            uint32 i = 0;
            uint32 end = static_cast<uint32>(text.size());
            auto&  glyphs = text.get<glyph_c>();
            float  ascent_y = 0.0f;
            float  descent_y = 0.0f;
            float  width = 0.0f;
            bool   wrap_enabled = line_wrap_width > 0.0f;

            TAV_ASSERT(text.size() < 0xffffffffull);

            while (i < end) {
                auto next_i = break_next_line(i, end, glyphs, ascent_y, descent_y, width, wrap_enabled, line_wrap_width);
                lines.emplace_back(text_line_c{i, next_i - i, ascent_y, descent_y, width});
                i = next_i;
            }
        }
    };

} // namespace tavros::renderer
