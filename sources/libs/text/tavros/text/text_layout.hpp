#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/text/rich_line.hpp>
#include <tavros/core/debug/unreachable.hpp>

namespace tavros::text
{

    /**
     * @brief Horizontal text alignment modes used during layout.
     */
    enum class text_align
    {
        left,    /// Align lines to the left edge.
        right,   /// Align lines to the right edge.
        center,  /// Center-align each line within the available width.
        justify, /// Stretch lines so that spaces expand to fill the full width.
    };

    /**
     * @brief Parameters controlling the text layout process.
     */
    struct layout_params
    {
        /// Target line width used for line breaking and alignment.
        float width = 0.0f;

        /// Multiplier applied to the computed line gap.
        float line_spacing = 1.0f;

        /// Horizontal alignment mode applied to each line.
        text_align align = text_align::left;
    };

    /**
     * @brief Static utility class responsible for breaking text into lines
     *        and computing per-glyph layout positions.
     */
    class text_layout : core::nonconstructable
    {
    public:
        /**
         * @brief Performs text line breaking and horizontal layout.
         *
         * Splits the input glyph sequence into lines according to the provided width,
         * applies the selected horizontal alignment, and computes per-glyph layout
         * positions. All coordinates are produced relative to the baseline origin (0, 0);
         * final positioning and transformations should be applied by the rendering stage.
         *
         * The algorithm does not assume any fixed vertical metrics or external position.
         * Line heights, ascents, descents, and spacing are computed dynamically based on
         * the font metrics of each glyph. Only horizontal constraints (line width) are used.
         *
         * @tparam GlyphParams   Parameter pack describing glyph-specific data.
         * @param glyphs         Mutable span of glyph data. Layout fields inside each glyph
         *                       will be updated in-place.
         * @param params         Layout parameters including target line width, line spacing
         *                       and alignment mode.
         *
         * @return Bounding box of the complete laid-out text relative to the origin.
         */
        template<class GlyphParams>
        static geometry::aabb2 layout(core::buffer_span<glyph_data_param<GlyphParams>> glyphs, const layout_params& params) noexcept
        {
            using glyph_t = glyph_data_param<GlyphParams>;
            geometry::aabb2 container;

            struct line_run
            {
                glyph_t* beg = nullptr;
                glyph_t* end = nullptr;

                float  width = 0.0f;
                float  right_spaces_width = 0.0f;
                float  ascent_y = 0.0f;
                float  descent_y = 0.0f;
                float  line_gap_y = 0.0f;
                uint32 gaps = 0;
            };

            auto break_line = [max_width = params.width](glyph_t* begin, glyph_t* end, line_run& line) -> glyph_t* {
                line = {};

                auto max_abs = [](float a, float b) { return std::abs(a) > std::abs(b) ? a : b; };

                if (begin >= end) {
                    return end;
                }

                glyph_t* it = begin;
                bool     is_first_word_in_line = true;
                uint32   prev_gaps_num = 0;
                bool     is_leading_spaces = true;

                while (it < end) {
                    line_run cur_word;
                    uint32   gaps_num = 0;
                    cur_word.beg = it;

                    // While is not space
                    while (it < end && !it->base.is_space) {
                        auto  scale = it->base.glyph_size;
                        auto& font_info = it->base.font->get_font_metrics();

                        cur_word.width += it->base.advance_x + it->base.kern_x;
                        cur_word.ascent_y = max_abs(cur_word.ascent_y, font_info.ascent_y * scale);
                        cur_word.descent_y = max_abs(cur_word.descent_y, font_info.descent_y * scale);
                        cur_word.line_gap_y = max_abs(cur_word.line_gap_y, font_info.line_gap_y * scale);
                        is_leading_spaces = false;

                        ++it;
                    }

                    // While space
                    bool new_line = false;
                    while (it < end && it->base.is_space) {
                        auto  scale = it->base.glyph_size;
                        auto& font_info = it->base.font->get_font_metrics();

                        cur_word.right_spaces_width += it->base.advance_x + it->base.kern_x;
                        cur_word.ascent_y = max_abs(cur_word.ascent_y, font_info.ascent_y * scale);
                        cur_word.descent_y = max_abs(cur_word.descent_y, font_info.descent_y * scale);
                        cur_word.line_gap_y = max_abs(cur_word.line_gap_y, font_info.line_gap_y * scale);
                        if (!is_leading_spaces) {
                            ++gaps_num;
                        }

                        if (it->base.is_new_line) {
                            new_line = true;
                            ++it;
                            break;
                        }

                        ++it;
                    }
                    cur_word.end = it;

                    // Add word to line
                    if (is_first_word_in_line) {
                        line = cur_word;

                        if (it == end) {
                            return it;
                        }

                    } else if (line.width + line.right_spaces_width + cur_word.width <= max_width) {
                        line.end = cur_word.end;
                        line.width += line.right_spaces_width + cur_word.width;
                        line.right_spaces_width = cur_word.right_spaces_width;
                        line.ascent_y = max_abs(line.ascent_y, cur_word.ascent_y);
                        line.descent_y = max_abs(line.descent_y, cur_word.descent_y);
                        line.line_gap_y = max_abs(line.line_gap_y, cur_word.line_gap_y);
                        line.gaps += prev_gaps_num;

                        if (it == end) {
                            return it;
                        }

                    } else {
                        return line.end;
                    }

                    if (new_line) {
                        return it;
                    }

                    is_first_word_in_line = false;
                    prev_gaps_num = gaps_num;
                }
            };

            // layout phase
            math::vec2 baseline_pos;

            line_run line;
            glyph_t* line_it = glyphs.begin();
            glyph_t* end = glyphs.end();
            bool     is_leading_spaces = true;

            // iterate lines
            line_it = break_line(line_it, end, line);
            baseline_pos.y = line.ascent_y;

            while (line.beg != nullptr) {
                baseline_pos.x = 0.0f;

                uint32 remaining_gaps = 0;
                bool   is_last_line = line_it == end;
                float  additional_space = 0.0f;
                switch (params.align) {
                case text_align::left:
                    break;
                case text_align::center:
                    baseline_pos.x += (params.width - line.width) / 2.0f;
                    break;
                case text_align::right:
                    baseline_pos.x += params.width - line.width;
                    break;
                case text_align::justify:
                    if (line.gaps != 0 && !is_last_line) {
                        remaining_gaps = line.gaps;
                        additional_space = (params.width - line.width) / static_cast<float>(remaining_gaps);
                    }
                    break;
                default:
                    TAV_UNREACHABLE();
                }

                // iterate glyphs
                for (auto* it = line.beg; it < line.end; ++it) {
                    auto& info = it->base.font->get_glyph_info(it->base.idx);
                    auto  scale = it->base.glyph_size;

                    auto layout_pos = baseline_pos + it->base.bearing - (math::vec2(0.0f, info.metrics.size.height)) * scale;
                    it->base.layout.min = layout_pos;
                    it->base.layout.max = layout_pos + info.metrics.size * scale;
                    it->base.bounds.min = math::vec2(baseline_pos.x, baseline_pos.y - line.ascent_y);
                    it->base.bounds.max = math::vec2(baseline_pos.x + it->base.advance_x, baseline_pos.y - line.descent_y);

                    if (it->base.is_space && remaining_gaps > 0) {
                        if (is_leading_spaces) {
                            baseline_pos.x += it->base.advance_x + it->base.kern_x;
                        } else {
                            it->base.layout.min.x += additional_space / 2.0f;
                            it->base.layout.max.x += additional_space / 2.0f;
                            it->base.bounds.max.x += additional_space;

                            baseline_pos.x += it->base.advance_x + it->base.kern_x + additional_space;
                            --remaining_gaps;
                        }
                    } else {
                        baseline_pos.x += it->base.advance_x + it->base.kern_x;
                        is_leading_spaces = false;
                    }

                    container.expand(it->base.bounds.min);
                    container.expand(it->base.bounds.max);
                }

                baseline_pos.y += (line.ascent_y - line.descent_y + line.line_gap_y) * params.line_spacing;
                line_it = break_line(line_it, end, line);
            }

            return container;
        }
    };

} // namespace tavros::text