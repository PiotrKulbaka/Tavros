#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/text/font/font.hpp>
#include <tavros/core/geometry.hpp>

namespace tavros::text
{
    using packed_rgba8888 = uint32; // RGBA8888

    struct glyph_rect
    {
        uint16 left = 0;
        uint16 top = 0;
        uint16 right = 0;
        uint16 bottom = 0;
    };

    struct glyph_instance
    {
        float           mat[3][2] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        glyph_rect      rect;
        packed_rgba8888 fill_color = 0;
        packed_rgba8888 outline_color = 0;
    };

    struct glyph_layout
    {
        const font*       font = nullptr;
        float             font_size = 0.0f;
        char32            codepoint = 0;
        font::glyph_index glyph_idx = 0; // Индекс в текущем грифте
        float             kern = 0.0f;   // Кернинг с глифом слева от текущего
        math::color       fill_color;
        math::color       outline_color;
        float             mat[3][2] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    };

    struct text_style
    {
        const font* font = nullptr;
        math::color fill_color = {1.0f, 1.0f, 1.0f, 1.0f};
        math::color outline_color = {0.0f, 0.0f, 0.0f, 1.0f};
        float       font_size = 21.0f;
    };

    struct char_range
    {
        uint32 begin = 0;
        uint32 end = 0;
    };

    class text_layout
    {
    public:
        void set_rect(geometry::aabb2 r);

        void layout(core::buffer_span<glyph_layout> glyphs);

        const geometry::aabb2& rect();

    private:
        geometry::aabb2 m_rect;
    };

    class rich_line
    {
    public:
        rich_line();
        ~rich_line();

        void set_text(core::u32string_view text, const text_style& initial_style);

        void set_style(const text_style& style, char_range range);

        void set_font(font* fnt, char_range range);

        void set_color(const math::color& color, char_range range);

        void set_outline_color(const math::color& color, char_range range);

        void set_font_size(float size, char_range range);

        text_layout& layouter();

        void layout();

        uint32 length();

        void fill_instances(core::buffer_span<glyph_instance> buf);

    private:
        text_layout                m_layouter;
        core::vector<glyph_layout> m_glyphs;
    };

} // namespace tavros::text