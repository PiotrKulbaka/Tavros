#include <tavros/renderer/text/font/font.hpp>

#include <algorithm>

namespace tavros::renderer
{

    font::font()
        : m_font_metrics(0.8f, -0.2f, 0.0f, 0.0f)
    {
        m_glyphs.emplace_back(0, atlas_rect_t{}, glyph_metrics{tavros::math::vec2(0.5f), tavros::math::vec2(0.0f), tavros::math::size2(0.4f, 0.5f)});
    }

    bool font::equals(const font& other) const noexcept
    {
        return this == &other;
    }

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

    float font::get_kerning(char32 left_codepoint, char32 right_codepoint) const noexcept
    {
        return get_kerning_internal(left_codepoint, right_codepoint);
    }

    math::isize2 font::glyph_bitmap_size(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix) const noexcept
    {
        if (0 != idx) {
            return glyph_bitmap_size_internal(idx, glyph_scale_pix, glyph_sdf_pad_pix);
        }

        auto        pad2 = static_cast<int32>(math::ceil(glyph_sdf_pad_pix)) * 2;
        const auto& g = get_glyph_info(idx);

        auto sz = g.metrics.size * glyph_scale_pix;
        return math::ivec2(
            static_cast<int32>(math::ceil(sz.width)) + pad2,
            static_cast<int32>(math::ceil(sz.height)) + pad2
        );
    }

    void font::bake_glyph_bitmap(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix, core::buffer_span<uint8> pixels, uint32 pixels_stride) const noexcept
    {
        if (0 != idx) {
            bake_glyph_bitmap_internal(idx, glyph_scale_pix, glyph_sdf_pad_pix, pixels, pixels_stride);
            return;
        }

        const auto& g = get_glyph_info(idx);

        auto  pad = static_cast<int32>(math::ceil(glyph_sdf_pad_pix));
        auto  pix_dist_scale = pad == 0 ? 256.0f : 128.0f / static_cast<float>(pad);
        auto* dst = pixels.data();

        auto sz = g.metrics.size * glyph_scale_pix;
        auto bitmap_sz = math::ivec2(
            static_cast<int32>(math::ceil(sz.width)) + pad * 2,
            static_cast<int32>(math::ceil(sz.height)) + pad * 2
        );

        auto cx = static_cast<float>(bitmap_sz.width) / 2.0f;
        auto cy = static_cast<float>(bitmap_sz.height) / 2.0f;
        auto dist_offset = std::min(cx, cy);
        auto aspect = sz.width / sz.height;

        // Gen null glyph with sdf (rhombus generation)
        for (int32 y = 0; y < bitmap_sz.height; ++y) {
            for (int32 x = 0; x < bitmap_sz.width; ++x) {
                float dx = std::abs(x - cx);
                float dy = std::abs(y - cy) * aspect;
                float dist = -(dx + dy) + dist_offset;

                auto val = static_cast<uint8>(std::clamp(dist * pix_dist_scale, 0.0f, 255.0f));
                dst[y * pixels_stride + x] = val;
            }
        }
    }

} // namespace tavros::renderer
