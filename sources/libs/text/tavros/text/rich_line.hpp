#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/geometry/aabb2.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/text/font/font.hpp>

namespace tavros::text
{

    struct text_style
    {
        const font* font = nullptr;
        math::color fill_color = {1.0f, 1.0f, 1.0f, 1.0f};
        math::color outline_color = {0.0f, 0.0f, 0.0f, 1.0f};
        float       font_size = 21.0f;
    };

    enum class text_align
    {
        left,
        right,
        center,
        justify,
    };

    struct layout_params
    {
        float      width = 0.0f;
        text_align align = text_align::left;
    };


    struct glyph_data_base
    {
        char32 codepoint = 0;
        bool   is_space = false;

        const font*       font = nullptr;
        font::glyph_index idx = 0;
        float             glyph_size = 0.0f;
        atlas_rect        rect;
        math::vec2        bearing;
        float             advance_x = 0.0f;
        float             kern_x = 0.0f;
        geometry::aabb2   layout;
        geometry::aabb2   bounds;
    };

    template<class GlyphParams>
    struct glyph_data_param
    {
        glyph_data_base base;
        GlyphParams     params;
    };

    template<>
    struct glyph_data_param<void>
    {
        glyph_data_base base;
    };


    template<class GlyphParams = void>
    class rich_line
    {
    public:
        using glyph_data = glyph_data_param<GlyphParams>;

    public:
        rich_line() noexcept = default;

        ~rich_line() noexcept = default;

        void set_text(core::u32string_view text, const text_style& style)
        {
            TAV_ASSERT(style.font != nullptr);

            m_glyphs.reserve(text.length());
            m_glyphs.clear(); // clear old data

            auto is_sp = [](char32 c) { return c == ' ' || c == '\t' || c == '\v' || c == '\r' || c == '\n' || c == '\f'; };

            for (auto it = text.begin(); it < text.end(); ++it) {
                auto cp = *it;

                glyph_data g;
                g.base.codepoint = cp;
                g.base.is_space = is_sp(cp);

                m_glyphs.push_back(g);
            }

            set_style(0, m_glyphs.size(), style);
        }

        void set_style(size_t begin, size_t end, const text_style& style)
        {
            TAV_ASSERT(style.font != nullptr);
            TAV_ASSERT(begin < m_glyphs.size());
            TAV_ASSERT(end <= m_glyphs.size());
            TAV_ASSERT(begin < end);

            for (auto i = begin; i < end; ++i) {
                auto& g = m_glyphs[i].base;

                auto& info = style.font->get_glyph_info(g.idx);

                g.font = style.font;
                g.idx = style.font->find_glyph(g.codepoint);
                g.glyph_size = style.font_size;
                g.rect = info.entry;
                g.bearing = info.metrics.bearing * style.font_size;
                g.advance_x = info.metrics.advance.x * style.font_size;

                auto next = i + 1;
                if (next != end) {
                    g.kern_x = style.font->get_kerning(g.idx, m_glyphs[next].base.idx) * g.glyph_size;
                } else if (next < m_glyphs.size() && style.font == m_glyphs[next].base.font) {
                    g.kern_x = style.font->get_kerning(g.idx, m_glyphs[next].base.idx) * g.glyph_size;
                } else {
                    g.kern_x = 0.0f;
                }
            }
        }

        void set_font(size_t begin, size_t end, font* fnt)
        {
            TAV_ASSERT(fnt != nullptr);
            TAV_ASSERT(begin < m_glyphs.size());
            TAV_ASSERT(end <= m_glyphs.size());
            TAV_ASSERT(begin < end);

            for (auto i = begin; i < end; ++i) {
                auto& g = m_glyphs[i].base;
                auto& info = fnt->get_glyph_info(g.idx);

                g.font = fnt;
                g.idx = fnt->find_glyph(g.codepoint);
                g.rect = info.entry;
                g.bearing = info.metrics.bearing * g.glyph_size;
                g.advance_x = info.metrics.advance.x * g.glyph_size;

                auto next = i + 1;
                if (next != end) {
                    g.kern_x = fnt->get_kerning(g.idx, m_glyphs[next].base.idx) * g.glyph_size;
                } else if (next < m_glyphs.size() && fnt == m_glyphs[next].base.font) {
                    g.kern_x = g.font->get_kerning(g.idx, m_glyphs[next].base.idx) * g.glyph_size;
                } else {
                    g.kern_x = 0.0f;
                }
            }
        }

        void set_font_size(size_t begin, size_t end, float size)
        {
            TAV_ASSERT(begin < m_glyphs.size());
            TAV_ASSERT(end <= m_glyphs.size());
            TAV_ASSERT(begin < end);

            for (auto i = begin; i < end; ++i) {
                auto& g = m_glyphs[i].base;
                auto& info = g.font->get_glyph_info(g.idx);

                g.glyph_size = size;
                g.bearing = info.metrics.bearing * size;
                g.advance_x = info.metrics.advance.x * size;
                auto next = i + 1;
                if (next < m_glyphs.size() && g.font == m_glyphs[next].base.font) {
                    g.kern_x = g.font->get_kerning(g.idx, m_glyphs[next].base.idx) * size;
                } else {
                    g.kern_x = 0.0f;
                }
            }
        }

        core::buffer_view<glyph_data> glyphs_view() const noexcept
        {
            return m_glyphs;
        }

        core::buffer_span<glyph_data> glyphs() noexcept
        {
            return m_glyphs;
        }

    private:
        core::vector<glyph_data> m_glyphs;
    };

} // namespace tavros::text