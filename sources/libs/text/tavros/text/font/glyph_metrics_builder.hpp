#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/text/font/font.hpp>
#include <tavros/core/geometry/aabb2.hpp>

namespace tavros::text
{

    class glyph_metrics_builder : core::noncopyable, core::nonmovable
    {
    public:
        glyph_metrics_builder(const font* fnt, char32 codepoint, float glyph_size)
            : m_font(fnt)
            , m_glyph_size(glyph_size)
            , m_gi(&fnt->get_glyph_info(fnt->find_glyph(codepoint)))
            , m_fm(&fnt->get_font_metrics())
        {
        }

        ~glyph_metrics_builder() noexcept = default;

        atlas_rect build_atlas_rect() const noexcept
        {
            return m_gi->entry;
        }

        geometry::aabb2 build_layout_rect() const noexcept
        {
            auto pad = m_fm->sdf_padding_pix * m_glyph_size;
            auto b = m_gi->metrics.bearing * m_glyph_size;
            return geometry::aabb2(
                -pad + b.x,
                -m_gi->metrics.size.y * m_glyph_size - pad + b.y,
                m_gi->metrics.size.x * m_glyph_size + pad + b.x,
                pad + b.y
            );
        }

        geometry::aabb2 build_bounds_rect() const noexcept
        {
            return geometry::aabb2(
                0.0f,
                -m_fm->ascent_y * m_glyph_size,
                m_gi->metrics.advance.x * m_glyph_size,
                -m_fm->descent_y * m_glyph_size
            );
        }

        float build_advance_x() const noexcept
        {
            return m_gi->metrics.advance.x * m_glyph_size;
        }

        float build_kern_x(char32 next_codepoint) const noexcept
        {
            return m_font->get_kerning(m_gi->codepoint, next_codepoint) * m_glyph_size;
        }

        float build_ascent_y() const noexcept
        {
            return m_fm->ascent_y * m_glyph_size;
        }

        float build_descent_y() const noexcept
        {
            return m_fm->descent_y * m_glyph_size;
        }

        float build_line_gap_y() const noexcept
        {
            return m_fm->line_gap_y * m_glyph_size;
        }

    private:
        const font*               m_font;
        float                     m_glyph_size;
        const font::glyph_info*   m_gi;
        const font::font_metrics* m_fm;
    };

} // namespace tavros::text