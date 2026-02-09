#include <tavros/text/font/font.hpp>

#include <algorithm>

namespace tavros::text
{

    font::glyph_index font::find_glyph(char32 codepoint) const noexcept
    {
        auto it = std::lower_bound(
            m_glyphs.begin(),
            m_glyphs.end(),
            codepoint,
            [](const glyph_info& g, char32 cp) { return g.codepoint < cp; }
        );

        if (it != m_glyphs.end() && it->codepoint == codepoint) {
            return static_cast<glyph_index>(it - m_glyphs.begin());
        }

        return 0;
    }

    const font::font_metrics& font::get_font_metrics() const noexcept
    {
        return m_font_metrics;
    }

    const font::glyph_info& font::get_glyph_info(glyph_index idx) const noexcept
    {
        TAV_ASSERT(idx < m_glyphs.size());
        return m_glyphs[idx];
    }

    float font::get_kerning(glyph_index left, glyph_index right) const noexcept
    {
        auto& l = get_glyph_info(left);
        auto& r = get_glyph_info(right);
        return get_kerning_internal(l.codepoint, r.codepoint);
    }

} // namespace tavros::text
