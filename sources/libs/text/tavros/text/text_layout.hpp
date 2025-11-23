#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/text/rich_line.hpp>
#include <tavros/core/debug/unreachable.hpp>

namespace tavros::text
{

    class text_layout : core::nonconstructable
    {
    public:
        template<class GlyphParams>
        static void layout(core::buffer_span<glyph_data_param<GlyphParams>> glyphs, const layout_params& params)
        {
            struct word_info
            {
                glyph_data_param<GlyphParams>* beg = nullptr;
                glyph_data_param<GlyphParams>* end = nullptr;

                float left_spaces_width = 0.0f;
                float word_width = 0.0f;
                float ascent_y = 0.0f;
                float descent_y = 0.0f;
                float line_gap_y = 0.0f;
            };
            core::vector<word_info> words;

            // 1. Split line by words
            {
                glyph_data_param<GlyphParams>*       it = glyphs.begin();
                const glyph_data_param<GlyphParams>* end = glyphs.end();

                while (it < end) {
                    word_info cur_word;

                    while (it < end && it->base.is_space) {
                        cur_word.left_spaces_width += it->base.advance_x;
                        ++it;
                    }

                    cur_word.beg = it;

                    while (it < end && !it->base.is_space) {
                        auto  scale = it->base.glyph_size;
                        auto& font_info = it->base.font->get_font_metrics();

                        // Update word width
                        cur_word.word_width += it->base.advance_x + it->base.kern_x;

                        // Update ascent
                        float candidate_ascent = font_info.ascent_y * scale;
                        if (cur_word.ascent_y < candidate_ascent) {
                            cur_word.ascent_y = candidate_ascent;
                        }

                        // Update descent
                        float candidate_descent = font_info.descent_y * scale;
                        if (std::abs(cur_word.descent_y) < std::abs(candidate_descent)) {
                            cur_word.descent_y = candidate_descent;
                        }

                        // Update line_gap
                        float candidate_line_gap = font_info.line_gap_y * scale;
                        if (cur_word.line_gap_y < candidate_line_gap) {
                            cur_word.line_gap_y = candidate_line_gap;
                        }

                        ++it;
                    }
                    cur_word.end = it;

                    if (cur_word.beg != cur_word.end) {
                        words.emplace_back(cur_word);
                    }
                }
            }

            struct line_info
            {
                word_info* beg = nullptr;
                word_info* end = nullptr;

                float line_width = 0.0f;
                float ascent_y = 0.0f;
                float descent_y = 0.0f;
                float line_gap_y = 0.0f;

                uint32 words_number = 0;
            };
            core::vector<line_info> lines;

            // 2. Split words by content width
            {
                auto*       it = words.data();
                const auto* end = words.data() + words.size();

                bool is_first_line = true;
                while (it < end) {
                    line_info cur_line;
                    cur_line.beg = it;

                    while (it < end) {
                        if (cur_line.line_width == 0.0f) {
                            // Is a new line. Always add the first word to the line

                            cur_line.line_width = it->word_width;
                            cur_line.ascent_y = it->ascent_y;
                            cur_line.descent_y = it->descent_y;
                            cur_line.line_gap_y = it->line_gap_y;

                            if (is_first_line && (cur_line.line_width + it->left_spaces_width <= params.width)) {
                                cur_line.line_width += it->left_spaces_width;
                            }

                        } else if (cur_line.line_width + it->left_spaces_width + it->word_width <= params.width) {
                            // Still on the current line
                            cur_line.line_width += it->left_spaces_width + it->word_width;

                            if (cur_line.ascent_y < it->ascent_y) {
                                cur_line.ascent_y = it->ascent_y;
                            }

                            if (std::abs(cur_line.descent_y) < std::abs(it->descent_y)) {
                                cur_line.descent_y = it->descent_y;
                            }

                            if (cur_line.line_gap_y < it->line_gap_y) {
                                cur_line.line_gap_y = it->line_gap_y;
                            }

                        } else {
                            // A new line is needed
                            break;
                        }

                        ++cur_line.words_number;
                        ++it;
                    }
                    cur_line.end = it;

                    is_first_line = false;
                    lines.push_back(cur_line);
                }
            }

            // 3. Ñalculate the glyphs layout
            {
                math::vec2 baseline_pos;

                auto* it = glyphs.begin();

                const auto* line_end = lines.data() + lines.size();
                bool        is_first_line = true;
                for (auto* line = lines.data(); line < line_end; ++line) {
                    baseline_pos.y += line->ascent_y;
                    baseline_pos.x = 0.0f;

                    float additional_space = 0.0f;

                    bool is_last_line = line + 1 >= line_end;

                    switch (params.align) {
                    case text_align::left:
                        break;
                    case text_align::center:
                        baseline_pos.x += (params.width - line->line_width) / 2.0f;
                        break;
                    case text_align::right:
                        baseline_pos.x += params.width - line->line_width;
                        break;
                    case text_align::justify:
                        if (line->words_number > 1 && !is_last_line) {
                            additional_space = (params.width - line->line_width) / static_cast<float>(line->words_number - 1);
                        }
                        break;
                    default:
                        TAV_UNREACHABLE();
                    }

                    bool is_first_word_in_line = true;
                    for (auto* word = line->beg; word < line->end; ++word) {
                        // If is not first word in the line
                        if (!is_first_word_in_line || is_first_line && is_first_word_in_line) {
                            baseline_pos.x += word->left_spaces_width;
                        }

                        if (!is_first_word_in_line) {
                            baseline_pos.x += additional_space;
                        }

                        for (auto* it = word->beg; it < word->end; ++it) {
                            auto& info = it->base.font->get_glyph_info(it->base.idx);
                            auto  scale = it->base.glyph_size;

                            auto layout_pos = baseline_pos + it->base.bearing - (math::vec2(0.0f, info.metrics.size.height)) * scale;
                            it->base.layout.min = layout_pos;
                            it->base.layout.max = layout_pos + info.metrics.size * scale;
                            it->base.bounds.min = math::vec2(baseline_pos.x, baseline_pos.y - line->ascent_y);
                            it->base.bounds.max = math::vec2(baseline_pos.x + it->base.advance_x, baseline_pos.y - line->descent_y);

                            baseline_pos.x += it->base.advance_x + it->base.kern_x;
                        }

                        is_first_word_in_line = false;
                    }

                    baseline_pos.y += -line->descent_y + line->line_gap_y;
                    is_first_line = false;
                }
            }
        }
    };

} // namespace tavros::text