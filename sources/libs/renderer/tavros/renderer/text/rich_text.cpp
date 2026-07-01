#include <tavros/renderer/text/rich_text.hpp>

namespace tavros::renderer
{

    rich_text::rich_text(font_ref initial_font, float initial_font_size) noexcept
        : m_dirty(true)
        , m_cur_font(initial_font)
        , m_cur_font_size(initial_font_size)
        , m_cur_fill_color(math::rgba8(255, 255, 255, 255))
        , m_cur_outline_color(math::rgba8(0, 0, 0, 255))
        , m_line_wrap_width(0.0f)
        , m_text_align(text_align::left)
        , m_line_spacing(1.0f)
    {
    }

    void rich_text::set_font(font_ref fnt) noexcept
    {
        m_cur_font = fnt;
    }

    void rich_text::set_font_size(float size_px) noexcept
    {
        m_cur_font_size = size_px;
    }

    void rich_text::set_fill_color(math::rgba8 color) noexcept
    {
        m_cur_fill_color = color;
    }

    void rich_text::set_outline_color(math::rgba8 color) noexcept
    {
        m_cur_outline_color = color;
    }

    void rich_text::set_line_spacing(float spacing) noexcept
    {
        m_line_spacing = spacing;
        m_dirty = true;
    }

    void rich_text::set_line_wrap_width(float width) noexcept
    {
        m_line_wrap_width = width;
        m_dirty = true;
    }

    void rich_text::set_text_align(text_align align) noexcept
    {
        m_text_align = align;
        m_dirty = true;
    }

    void rich_text::append_text(core::string_view str)
    {
        text_builder::append_text(m_text, str, *m_cur_font, m_cur_font_size);
        m_text.view<glyph_style_c>().each([&](glyph_style_c& s) {
            s.fill_color = m_cur_fill_color;
            s.outline_color = m_cur_outline_color;
        });
        m_dirty = true;
    }

    void rich_text::clear() noexcept
    {
        m_text.clear();
        m_dirty = true;
    }

    void rich_text::shrink_to_fit()
    {
        m_text.shrink_to_fit();
    }

    geometry::aabb2 rich_text::layout() noexcept
    {
        if (m_dirty) {
            m_aabb = text_layouter::layout(m_text, m_line_wrap_width, m_text_align, m_line_spacing);
            m_dirty = false;
        }
        return m_aabb;
    }

    const text_archetype& rich_text::text() noexcept
    {
        layout();
        return m_text;
    }

} // namespace tavros::renderer
