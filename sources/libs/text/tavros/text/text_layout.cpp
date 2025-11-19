#include <tavros/text/text_layout.hpp>

#include <tavros/core/utf8.hpp>

namespace
{

    bool is_space(char32 c)
    {
        return c == ' ' || c == '\t' || c == '\v' || c == '\r' || c == '\n' || c == '\f';
    }


    uint32 pack_color(tavros::math::vec4 color)
    {
        auto r = static_cast<uint32>(color.r * 255.0f) & 0xffu;
        auto g = static_cast<uint32>(color.g * 255.0f) & 0xffu;
        auto b = static_cast<uint32>(color.b * 255.0f) & 0xffu;
        auto a = static_cast<uint32>(color.a * 255.0f) & 0xffu;
        return (r << 24) | (g << 16) | (b << 8) | a;
    }


} // namespace

namespace tavros::text
{

    void text_layout::set_rect(geometry::aabb2 r)
    {
        m_rect = r;
    }

    void text_layout::layout(core::buffer_span<glyph_layout> glyphs)
    {
        struct word_info
        {
            glyph_layout* beg = nullptr;
            glyph_layout* end = nullptr;

            float left_spaces_width = 0.0f;
            float word_width = 0.0f;
            float ascent_y = 0.0f;
            float descent_y = 0.0f;
            float line_gap_y = 0.0f;
        };
        core::vector<word_info> words;

        // 1. Split line by words
        {
            glyph_layout*       it = glyphs.begin();
            const glyph_layout* end = glyphs.end();

            while (it < end) {
                word_info cur_word;

                while (it < end && is_space(it->codepoint)) {
                    // Update word spaces width
                    cur_word.left_spaces_width += it->font->get_glyph_info(it->glyph_idx).metrics.advance.x * it->font_size;
                    ++it;
                }

                cur_word.beg = it;

                while (it < end && !is_space(it->codepoint)) {
                    auto& info = it->font->get_glyph_info(it->glyph_idx);
                    auto& font_info = it->font->get_font_metrics();

                    auto scale = it->font_size;
                    auto kern = 0.0f;

                    // Calc kerning
                    auto next_it = it + 1;
                    if (next_it < end && !is_space(next_it->codepoint)) {
                        auto* font_cur = it->font;
                        auto* font_next = next_it->font;

                        float kern_l;
                        float kern_r;

                        if (font_cur == font_next) {
                            kern_l = kern_r = font_cur->get_kerning(it->glyph_idx, next_it->glyph_idx);
                        } else {
                            kern_l = font_cur->get_kerning(it->glyph_idx, next_it->glyph_idx);
                            kern_r = font_next->get_kerning(it->glyph_idx, next_it->glyph_idx);
                        }

                        kern = kern_l * it->font_size * 0.5f + kern_r * next_it->font_size * 0.5f;
                        it->kern = kern;
                    }

                    // Update word width
                    cur_word.word_width += info.metrics.advance.x * scale + kern;

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
            bool  use_first_space = false;
        };
        core::vector<line_info> lines;

        // 2. Split words by content width
        {
            auto*       it = words.data();
            const auto* end = words.data() + words.size();

            auto max_width = m_rect.size().width;

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

                        if (is_first_line && (cur_line.line_width + it->left_spaces_width <= max_width)) {
                            cur_line.line_width += it->left_spaces_width;
                            cur_line.use_first_space = true;
                        }

                    } else if (cur_line.line_width + it->left_spaces_width + it->word_width <= max_width) {
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

                    ++it;
                }
                cur_line.end = it;

                is_first_line = false;
                lines.push_back(cur_line);
            }
        }

        // 3. Ñalculate the glyphs layout
        {
            math::vec2 pen = {m_rect.left, m_rect.top};

            auto* it = glyphs.begin();

            const auto* line_end = lines.data() + lines.size();
            bool        is_first_line = true;
            for (auto* line = lines.data(); line < line_end; ++line) {
                pen.x = m_rect.left;
                pen.y += line->ascent_y;

                bool is_first_word_in_line = true;
                for (auto* word = line->beg; word < line->end; ++word) {
                    // If is not first word in the line
                    if (!is_first_word_in_line || line->use_first_space) {
                        pen.x += word->left_spaces_width;
                    }

                    for (auto* it = word->beg; it < word->end; ++it) {
                        auto& info = it->font->get_glyph_info(it->glyph_idx);
                        auto  scale = it->font_size;

                        it->mat[0][0] = (info.metrics.size.width + info.entry.sdf_pad * 2.0f) * scale;
                        it->mat[1][1] = (info.metrics.size.height + info.entry.sdf_pad * 2.0f) * scale;
                        // it->mat[1][0] = -g.metrics.size.height * scale / 4.0f;

                        // Ïîçèöèÿ quad’a (ó÷èòûâàåì bearing)
                        it->mat[2][0] = pen.x + (info.metrics.bearing.x - info.entry.sdf_pad) * scale;
                        it->mat[2][1] = pen.y + (info.metrics.bearing.y - info.metrics.size.height - info.entry.sdf_pad) * scale;

                        pen.x += info.metrics.advance.x * scale + it->kern;
                    }

                    is_first_word_in_line = false;
                }

                pen.y += -line->descent_y + line->line_gap_y * 2.0f;
                is_first_line = false;
            }
        }
    }

    const geometry::aabb2& text_layout::rect()
    {
        return m_rect;
    }


    rich_line::rich_line()
    {
    }

    rich_line::~rich_line()
    {
    }

    void rich_line::set_text(core::u32string_view text, const text_style& style)
    {
        TAV_ASSERT(style.font != nullptr);

        m_glyphs.reserve(text.length());
        m_glyphs.clear(); // clear old data

        for (auto it = text.begin(); it < text.end(); ++it) {
            auto cp = *it;
            auto idx = style.font->find_glyph(cp);

            glyph_layout g;
            g.font = style.font;
            g.font_size = style.font_size;
            g.codepoint = cp;
            g.glyph_idx = idx;
            g.fill_color = style.fill_color;
            g.outline_color = style.outline_color;

            m_glyphs.push_back(g);
        }
    }

    void rich_line::set_style(const text_style& style, char_range range)
    {
        TAV_ASSERT(style.font != nullptr);
        TAV_ASSERT(range.begin < m_glyphs.size());
        TAV_ASSERT(range.end <= m_glyphs.size());
        TAV_ASSERT(range.begin < range.end);

        for (auto i = range.begin; i < range.end; ++i) {
            auto& g = m_glyphs[i];
            g.font = style.font;
            g.font_size = style.font_size;
            g.glyph_idx = style.font->find_glyph(g.codepoint);
            g.fill_color = style.fill_color;
            g.outline_color = style.outline_color;
        }
    }

    void rich_line::set_font(font* fnt, char_range range)
    {
        TAV_ASSERT(fnt != nullptr);
        TAV_ASSERT(range.begin < m_glyphs.size());
        TAV_ASSERT(range.end <= m_glyphs.size());
        TAV_ASSERT(range.begin < range.end);

        for (auto i = range.begin; i < range.end; ++i) {
            m_glyphs[i].font = fnt;
            m_glyphs[i].glyph_idx = fnt->find_glyph(m_glyphs[i].codepoint);
        }
    }

    void rich_line::set_color(const math::color& color, char_range range)
    {
        TAV_ASSERT(range.begin < m_glyphs.size());
        TAV_ASSERT(range.end <= m_glyphs.size());
        TAV_ASSERT(range.begin < range.end);

        for (auto i = range.begin; i < range.end; ++i) {
            m_glyphs[i].fill_color = color;
        }
    }

    void rich_line::set_outline_color(const math::color& color, char_range range)
    {
        TAV_ASSERT(range.begin < m_glyphs.size());
        TAV_ASSERT(range.end <= m_glyphs.size());
        TAV_ASSERT(range.begin < range.end);

        for (auto i = range.begin; i < range.end; ++i) {
            m_glyphs[i].outline_color = color;
        }
    }

    void rich_line::set_font_size(float size, char_range range)
    {
        TAV_ASSERT(range.begin < m_glyphs.size());
        TAV_ASSERT(range.end <= m_glyphs.size());
        TAV_ASSERT(range.begin < range.end);

        for (auto i = range.begin; i < range.end; ++i) {
            m_glyphs[i].font_size = size;
        }
    }

    text_layout& rich_line::layouter()
    {
        return m_layouter;
    }

    void rich_line::layout()
    {
        m_layouter.layout(m_glyphs);
    }

    uint32 rich_line::length()
    {
        return static_cast<uint32>(m_glyphs.size());
    }

    void rich_line::fill_instances(core::buffer_span<glyph_instance> buf)
    {
        TAV_ASSERT(buf.size() == m_glyphs.size());

        auto* dst = buf.begin();
        for (auto src = m_glyphs.begin(); src < m_glyphs.end(); ++src) {
            auto& info = src->font->get_glyph_info(src->glyph_idx);
            std::memcpy(dst->mat, src->mat, sizeof(float) * 3 * 2);
            dst->rect.left = info.entry.left;
            dst->rect.top = info.entry.top;
            dst->rect.right = info.entry.right;
            dst->rect.bottom = info.entry.bottom;
            dst->fill_color = pack_color(src->fill_color);
            dst->outline_color = pack_color(src->outline_color);

            ++dst;
        }
    }

} // namespace tavros::text