#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/archetype.hpp>
#include <tavros/core/utf8.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/renderer/components/all.hpp>
#include <tavros/renderer/text/glyph_data.hpp>
#include <tavros/renderer/text/text_line_breaker.hpp>

#include <cwctype>

namespace tavros::renderer
{

    /**
     * @brief Horizontal text alignment modes used during layout.
     */
    enum class text_align
    {
        left,   /// Align lines to the left edge.
        right,  /// Align lines to the right edge.
        center, /// Center-align each line within the available width.
    };

    /**
     * @brief Static utility for positioning glyphs into 2D layout.
     *
     * Computes per-glyph positions (pos2_c) from glyph metrics and line data.
     * Supports horizontal alignment and configurable line spacing.
     *
     * Coordinate system:
     *  - Baseline-oriented.
     *  - Y axis increases downwards.
     *  - All resulting Y coordinates are non-negative.
     *  - Layout always starts at Y = 0.
     *
     * Horizontal alignment model:
     *
     *  left:
     *      X range: [0 ................ width]
     *      0 ---------------------------------> +X
     *      |***************|
     *
     *  center:
     *      X range: [-width/2 .......... +width/2]
     *      <----------- 0 ----------->
     *           |***************|
     *
     *  right:
     *      X range: [-width ............ 0]
     *      <--------------------------- 0
     *                   |***************|
     *
     *  Alignment is applied per line independently.
     *
     * Vertical spacing model:
     *  - ascent_y and descent_y are treated as signed metrics.
     *  - Line height = abs(descent_y - ascent_y).
     *  - Extra spacing is applied as: line_height * (line_spacing - 1)
     *
     *  - Y strictly increases from top to bottom.
     *
     * Notes:
     *  - step_x in glyph_c is assumed precomputed (including kerning).
     *  - No shaping is performed.
     *  - Returned AABB encloses the full positioned text block.
     *
     * Non-instantiable. All functionality is provided via static methods.
     */
    class text_layouter : core::nonconstructable
    {
    public:
        /**
         * @brief Layout using precomputed line metadata.
         *
         * Positions glyphs according to provided text_line_c data.
         * Does not perform line breaking.
         *
         * Guarantees:
         *  - pos2_c is assigned for every glyph in each line.
         *  - Horizontal alignment is applied per line.
         *  - Returned AABB encloses all positioned glyph baselines.
         *
         * @tparam Text  Archetype containing glyph_c and pos2_c.
         * @tparam Lines Archetype containing text_line_c.
         * @param text   Glyph container.
         * @param lines  Precomputed line metadata.
         * @param align  Horizontal alignment mode.
         * @param line_spacing Line spacing multiplier (1.0 = default line height).
         *
         * @return Bounding box of the laid out text block or invalid bbox if lines.size() is zero.
         */
        template<
            core::ArchetypeWith<glyph_c, pos2_c> Text,
            core::ArchetypeWith<text_line_c>     Lines>
        static geometry::aabb2 layout(Text& text, Lines& lines, text_align align = text_align::left, float line_spacing = 1.0f) noexcept
        {
            if (lines.size() == 0) {
                return {};
            }

            // Computes horizontal offset for a line depending on alignment mode
            auto shift_x = [align](float line_width) -> float {
                switch (align) {
                case text_align::left:
                    return 0.0f;
                case text_align::center:
                    return -line_width / 2.0f;
                case text_align::right:
                    return -line_width;
                default:
                    TAV_UNREACHABLE();
                }
            };

            float max_width = 0.0f;
            float y = 0.0f;

            lines.view<const text_line_c>().each([&](const auto& line) {
                float x = shift_x(line.width);
                y += math::abs(line.ascent_y);
                if (max_width < line.width) {
                    max_width = line.width;
                }
                TAV_ASSERT(static_cast<size_t>(line.first_glyph_index + line.glyph_count) <= text.size());
                text.view<const glyph_c, pos2_c>().each_n(line.first_glyph_index, line.glyph_count, [&](const auto& g, auto& p) {
                    p.set(x, y);
                    x += g.step_x;
                });
                y += math::abs(line.descent_y) + math::abs(line.descent_y - line.ascent_y) * (line_spacing - 1.0f);
            });

            auto& last_line = lines.get<text_line_c>().back();
            auto  min_x = shift_x(max_width);
            auto  max_x = min_x + max_width;
            auto  max_y = y - math::abs(last_line.descent_y - last_line.ascent_y) * (line_spacing - 1.0f);

            return {min_x, 0.0f, max_x, max_y};
        }

        /**
         * @brief Layout with automatic line breaking.
         *
         * Performs sequential line breaking using text_line_breaker
         * and positions glyphs accordingly.
         *
         * Wrapping behavior:
         *  - If line_wrap_width <= 0, wrapping is disabled.
         *  - Otherwise, lines are broken to satisfy width constraint.
         *
         * Guarantees:
         *  - pos2_c is assigned for all glyphs.
         *  - Horizontal alignment is applied per computed line.
         *  - step_x is assumed precomputed and unchanged.
         *  - Returned AABB encloses the final layout.
         *
         * Preconditions:
         *  - text.size() < 2^32
         *
         * @tparam Text Archetype containing glyph_c and pos2_c.
         * @param text  Glyph container.
         * @param line_wrap_width Maximum allowed line width (0 disables wrapping).
         * @param align Horizontal alignment mode.
         * @param line_spacing Line spacing multiplier (1.0 = default line height).
         *
         * @return Bounding box of the laid out text block or invalid bbox if text.size() is zero.
         */
        template<core::ArchetypeWith<glyph_c, pos2_c> Text>
        static geometry::aabb2 layout(Text& text, float line_wrap_width = 0.0f, text_align align = text_align::left, float line_spacing = 1.0f) noexcept
        {
            if (text.size() == 0) {
                return {};
            }

            auto shift_x = [align](float line_width) -> float {
                switch (align) {
                case text_align::left:
                    return 0.0f;
                case text_align::center:
                    return -line_width / 2.0f;
                case text_align::right:
                    return -line_width;
                default:
                    TAV_UNREACHABLE();
                }
            };

            uint32 i = 0;
            uint32 end = static_cast<uint32>(text.size());
            float  y = 0.0f;
            float  ascent_y = 0.0f;
            float  descent_y = 0.0f;
            float  width = 0.0f;
            auto&  glyphs = text.get<glyph_c>();

            TAV_ASSERT(text.size() < 0xffffffffull);

            float max_width = 0.0f;
            auto  wrap = line_wrap_width > 0.0f;

            while (i < end) {
                auto  next_i = text_line_breaker::break_next_line(i, end, glyphs, ascent_y, descent_y, width, wrap, line_wrap_width);
                float x = shift_x(width);
                y += math::abs(ascent_y);

                if (max_width < width) {
                    max_width = width;
                }

                text.view<const glyph_c, pos2_c>().each_n(i, next_i - i, [&](const auto& g, auto& p) {
                    p.set(x, y);
                    x += g.step_x;
                });

                i = next_i;
                y += math::abs(descent_y) + math::abs(descent_y - ascent_y) * (line_spacing - 1.0f);
            }

            auto min_x = shift_x(max_width);
            auto max_x = min_x + max_width;
            auto max_y = y - math::abs(descent_y - ascent_y) * (line_spacing - 1.0f);

            return {min_x, 0.0f, max_x, max_y};
        }
    };

} // namespace tavros::renderer