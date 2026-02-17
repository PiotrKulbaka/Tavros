#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/utf8.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/geometry/aabb2.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/text/glyph_data.hpp>
#include <tavros/text/font/glyph_metrics_builder.hpp>

#include <cwctype>

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


    class rich_text
    {
    private:
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

        static void update_step_x(glyph_c& g, char32 next_codepoint, bool next_font_is_same) noexcept
        {
            auto gb = glyph_metrics_builder(g.font, normalize_space(g.codepoint), g.glyph_size);
            auto step_x = gb.build_advance_x();
            if (next_font_is_same) {
                step_x += gb.build_kern_x(next_codepoint);
            }
            g.step_x = step_x;
        }

        static char32 normalize_space(char32 ch) noexcept
        {
            return std::iswspace(ch) ? static_cast<char32>(' ') : ch;
        }

    public:
        template<core::ArchetypeWith<glyph_c, atlas_rect_c, rect_layout_c, rect_bounds_c> Text>
        static void set_text(Text& text, core::string_view str, const font* fnt, float font_size)
        {
            text.clear();
            append_text(text, str, fnt, font_size);
        }

        template<core::ArchetypeWith<glyph_c, atlas_rect_c, rect_layout_c, rect_bounds_c> Text>
        static void append_text(Text& text, core::string_view str, const font* fnt, float font_size)
        {
            TAV_ASSERT(fnt != nullptr);

            auto codepoints = convert_to_codepoints(str);

            text.reserve(text.size() + codepoints.size());

            auto cp_size = codepoints.size();
            if (cp_size > 0) {
                // Update kerning of the last glyph if the new text is appended to existing text and uses the same font.
                if (text.size() > 0) {
                    glyph_c& last_g = text.get<glyph_c>().back();
                    update_step_x(last_g, codepoints[0], last_g.font->equals(*fnt));
                }

                for (size_t i = 0; i < cp_size; ++i) {
                    auto cp = codepoints[i];
                    auto is_space = static_cast<bool>(std::iswspace(cp));
                    auto gb = glyph_metrics_builder(fnt, normalize_space(cp), font_size);
                    auto step_x = gb.build_advance_x() + (i + 1 < cp_size ? gb.build_kern_x(codepoints[i + 1]) : 0.0f);
                    auto is_new_line = static_cast<bool>(static_cast<char32>('\n') == cp);

                    text.emplace_back(
                        glyph_c{fnt, cp, font_size, step_x, is_space, is_new_line},
                        atlas_rect_c{gb.build_atlas_rect()},
                        rect_layout_c{gb.build_layout_rect()},
                        rect_bounds_c{gb.build_bounds_rect()}
                    );
                }
            }
        }

        template<
            core::ArchetypeWith<glyph_c, atlas_rect_c, rect_layout_c, rect_bounds_c> Text>
        static void set_style(Text& text, size_t first, size_t count, const font* fnt, float font_size)
        {
            TAV_ASSERT(fnt != nullptr);
            TAV_ASSERT(first <= text.size());
            TAV_ASSERT(first + count <= text.size());

            if (count == 0) {
                return;
            }

            auto&  glyphs = text.get<glyph_c>();
            size_t end = first + count;

            text.view<glyph_c, atlas_rect_c, rect_layout_c, rect_bounds_c>()
                .each_n_indexed(first, count, [&](size_t i, glyph_c& g, atlas_rect_c& r, rect_layout_c& l, rect_bounds_c& b) {
                    auto gb = glyph_metrics_builder(fnt, normalize_space(g.codepoint), font_size);
                    g.font = fnt;
                    g.glyph_size = font_size;
                    if (i + 1 < end) {
                        g.step_x = gb.build_advance_x() + gb.build_kern_x(glyphs[i + 1].codepoint);
                    }

                    r = atlas_rect_c{gb.build_atlas_rect()};
                    l = rect_layout_c{gb.build_layout_rect()};
                    b = rect_bounds_c{gb.build_bounds_rect()};
                });

            // Update tail
            if (first > 0) {
                glyph_c& g = text.get<glyph_c>()[first - 1];
                update_step_x(g, text.get<glyph_c>()[first].codepoint, g.font->equals(*fnt));
            }

            // Update head
            if (end < text.size()) {
                glyph_c& g = text.get<glyph_c>()[end - 1];
                update_step_x(g, text.get<glyph_c>()[end].codepoint, g.font->equals(*fnt));
            } else {
                glyph_c& g = text.get<glyph_c>()[end - 1];
                update_step_x(g, 0, false);
            }
        }

        template<
            core::ArchetypeWith<glyph_c, rect_bounds_c>     Text,
            core::ArchetypeWith<text_line_c, rect_bounds_c> Lines>
        static void break_lines(Text& text, Lines& lines, float line_wrap_width = 0.0f)
        {
            const bool wrap_enabled = line_wrap_width > 0.0f;
            auto&      glyphs = text.get<glyph_c>();
            auto&      bounds = text.get<rect_bounds_c>();

            auto break_next_line = [&](uint32 begin, uint32 end, text_line_c& line, rect_bounds_c& rect_bounds) -> uint32 {
                line = {};
                rect_bounds = {};

                TAV_ASSERT(begin <= end);
                if (begin >= end) {
                    return end;
                }

                uint32 i = begin;
                bool   is_first_word_in_line = true;
                uint32 prev_gaps_num = 0;
                bool   is_leading_spaces = true;

                while (i < end) {
                    text_line_c cur_word;
                    uint32      gaps_num = 0;
                    cur_word.first_glyph_index = i;
                    cur_word.max_line_width = line_wrap_width;
                    float spaces_width = 0.0f;
                    float content_width = 0.0f;

                    // While is not space
                    while (i < end && !glyphs[i].is_space) {
                        content_width += glyphs[i].step_x;
                        if (cur_word.ascent_y > bounds[i].min.y) {
                            cur_word.ascent_y = bounds[i].min.y;
                        }
                        if (cur_word.descent_y < bounds[i].max.y) {
                            cur_word.descent_y = bounds[i].max.y;
                        }
                        is_leading_spaces = false;
                        ++i;
                    }

                    // While space
                    bool new_line = false;
                    while (i < end && glyphs[i].is_space) {
                        spaces_width += glyphs[i].step_x;
                        if (cur_word.ascent_y > bounds[i].min.y) {
                            cur_word.ascent_y = bounds[i].min.y;
                        }
                        if (cur_word.descent_y < bounds[i].max.y) {
                            cur_word.descent_y = bounds[i].max.y;
                        }
                        if (!is_leading_spaces) {
                            ++gaps_num;
                        }

                        if (glyphs[i].is_new_line) {
                            new_line = true;
                            ++i;
                            break;
                        }

                        ++i;
                    }
                    cur_word.glyph_count = i - cur_word.first_glyph_index;

                    // Add word to line
                    if (is_first_word_in_line) {
                        line = cur_word;
                        line.content_width = spaces_width + content_width;

                        rect_bounds.min.x = 0.0f;
                        rect_bounds.min.y = line.ascent_y;
                        rect_bounds.max.x = line.content_width;
                        rect_bounds.max.y = line.descent_y;

                        if (i == end) {
                            line.ends_with_newline = true;
                            return end;
                        }

                    } else if (!wrap_enabled || (line.content_width + line.trailing_whitespace_width + content_width <= line_wrap_width)) {
                        line.glyph_count += cur_word.glyph_count;
                        line.content_width += line.trailing_whitespace_width + content_width;
                        line.trailing_whitespace_width = spaces_width;
                        if (line.ascent_y > cur_word.ascent_y) {
                            line.ascent_y = cur_word.ascent_y;
                        }
                        if (line.descent_y < cur_word.descent_y) {
                            line.descent_y = cur_word.descent_y;
                        }
                        line.inter_word_whitespace_count += prev_gaps_num;

                        rect_bounds.min.x = math::min(rect_bounds.min.x, 0.0f);
                        rect_bounds.min.y = math::min(rect_bounds.min.y, line.ascent_y);
                        rect_bounds.max.x = math::max(rect_bounds.max.x, line.content_width + line.trailing_whitespace_width);
                        rect_bounds.max.y = math::max(rect_bounds.max.y, line.descent_y);

                        if (i == end) {
                            line.ends_with_newline = true;
                            return end;
                        }
                    } else {
                        return line.first_glyph_index + line.glyph_count;
                    }

                    if (new_line) {
                        line.ends_with_newline = true;
                        return i;
                    }

                    is_first_word_in_line = false;
                    prev_gaps_num = gaps_num;
                }
            };

            lines.clear();

            size_t line_i = 0;
            size_t end = text.size();
            while (line_i != end) {
                text_line_c   line{};
                rect_bounds_c rect_bounds{};
                line_i = break_next_line(line_i, end, line, rect_bounds);
                lines.emplace_back(line, rect_bounds);
            }
        }

        template<
            core::ArchetypeWith<glyph_c, rect_bounds_c, position_c>     Text,
            core::ArchetypeWith<text_line_c, rect_bounds_c, position_c> Lines>
        static void layout(Text& text, Lines& lines, text_align align = text_align::left, float line_spacing = 1.0f) noexcept
        {
            auto leading_x = [align](const text_line_c& line) {
                switch (align) {
                case text_align::left:
                    return 0.0f;
                case text_align::center:
                    return (line.max_line_width - line.content_width) / 2.0f;
                case text_align::right:
                    return line.max_line_width - line.content_width;
                case text_align::justify:
                    return 0.0f;
                default:
                    TAV_UNREACHABLE();
                }
            };

            if (align == text_align::justify) {
                // For the alignment algorithm, the recalculation of coordinates is a bit more complicated, so it is calculated separately
                float y = 0.0f;
                lines.view<const text_line_c, rect_bounds_c, position_c>()
                    .each_indexed([&](size_t line_i, const auto& line, auto& line_bounds, auto& line_pos) {
                        float x = leading_x(line);
                        auto  is_first_line = line_i == 0;
                        y += math::abs(line.ascent_y) * (is_first_line ? 1.0f : line_spacing);

                        line_bounds.max.x = 0.0f;

                        uint32 remaining_gaps = 0;
                        float  additional_space = 0.0f;
                        bool   is_leading_spaces = true;
                        if (line.inter_word_whitespace_count != 0 && !line.ends_with_newline) {
                            remaining_gaps = line.inter_word_whitespace_count;
                            additional_space = (line.max_line_width - line.content_width) / static_cast<float>(remaining_gaps);
                        }

                        line_pos.set(x, y);
                        text.view<const glyph_c, rect_bounds_c, position_c>()
                            .each_n(line.first_glyph_index, line.glyph_count, [&](const auto& glyph, auto& glyph_bounds, auto& glyph_pos) {
                                glyph_pos.set(x, y);
                                x += glyph.step_x;
                                line_bounds.max.x += glyph.step_x;
                                if (glyph.is_space && remaining_gaps > 0) {
                                    if (!is_leading_spaces) {
                                        glyph_bounds.max.x += additional_space;
                                        line_bounds.max.x += additional_space;
                                        x += additional_space;
                                        --remaining_gaps;
                                    }
                                } else {
                                    is_leading_spaces = false;
                                }
                            });
                        y += math::abs(line.descent_y) * line_spacing;
                    });
            } else {
                // For other alignment modes, the coordinates are calculated in a single pass.
                float y = 0.0f;
                lines.view<const text_line_c, position_c>()
                    .each_indexed([&](size_t line_i, const auto& line, auto& line_pos) {
                        float x = leading_x(line);
                        auto  is_first_line = line_i == 0;
                        y += math::abs(line.ascent_y) * (is_first_line ? 1.0f : line_spacing);
                        line_pos.set(x, y);
                        text.view<const glyph_c, position_c>()
                            .each_n(line.first_glyph_index, line.glyph_count, [&](const auto& glyph, auto& glyph_pos) {
                                glyph_pos.set(x, y);
                                x += glyph.step_x;
                            });
                        y += math::abs(line.descent_y) * line_spacing;
                    });
            }
        }
    };

} // namespace tavros::text