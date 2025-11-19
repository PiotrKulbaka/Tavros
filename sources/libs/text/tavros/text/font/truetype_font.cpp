#include <tavros/text/font/truetype_font.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/defines.hpp>
#include <tavros/core/math/functions/clamp.hpp>
#include <stb/stb_truetype.h>

namespace
{
    tavros::core::logger logger("truetype_font");
}

namespace tavros::text
{

    struct truetype_font::impl
    {
        stbtt_fontinfo info;
    };

    truetype_font::truetype_font() noexcept
        : m_font_data(nullptr)
        , m_is_init(false)
        , m_scale(0.0f)
    {
    }

    truetype_font::~truetype_font()
    {
        shutdown();
    }

    void truetype_font::init(core::dynamic_buffer<uint8> font_data, core::buffer_view<codepoint_range> codepoint_ranges) noexcept
    {
        if (m_is_init) {
            ::logger.error("Font already initialized");
            return;
        }

#if TAV_DEBUG
        // Verify ranges
        TAV_ASSERT(codepoint_ranges.size() > 0);
        TAV_ASSERT(codepoint_ranges[0].first_codepoint <= codepoint_ranges[0].last_codepoint); // Free range is not available
        for (size_t i = 1; i < codepoint_ranges.size(); ++i) {
            const auto& prev = codepoint_ranges[i - 1];
            const auto& curr = codepoint_ranges[i];

            char32 prev_end = prev.last_codepoint;

            TAV_ASSERT(curr.first_codepoint <= curr.last_codepoint); // Free range is not available
            TAV_ASSERT(curr.first_codepoint >= prev_end);            // There should be no overlapping of ranges
            TAV_ASSERT(curr.first_codepoint > prev.first_codepoint); // The ranges are increasing
        }
#endif

        if (stbtt_InitFont(&m_impl->info, font_data.data(), 0) == 0) {
            ::logger.error("Failed to init font data");
            return;
        }

        m_font_data = std::move(font_data);

        int32 ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&m_impl->info, &ascent, &descent, &line_gap);
        m_scale = stbtt_ScaleForPixelHeight(&m_impl->info, 1.0f);

        m_font_metrics = {static_cast<float>(ascent) * m_scale, static_cast<float>(descent) * m_scale, static_cast<float>(line_gap) * m_scale};

        // Null glyph
        m_glyphs.emplace_back(0, atlas_entry{}, glyph_metrics{math::vec2(0.5f), math::vec2(0.0f), math::size2(0.4f, 0.5f)});

        for (auto& range : codepoint_ranges) {
            char32 beg = range.first_codepoint;
            char32 end = range.last_codepoint + 1;

            // A null codepoint has already been added above so skip it
            if (0 == beg) {
                beg = 1;
            }

            for (char32 cp = beg; cp < end; ++cp) {
                int32 glyph_idx = stbtt_FindGlyphIndex(&m_impl->info, cp);
                if (glyph_idx == 0) {
                    continue;
                }

                int32 advance_width = 0, left_side_bearing = 0;
                stbtt_GetGlyphHMetrics(&m_impl->info, glyph_idx, &advance_width, &left_side_bearing);

                math::vec2  advance(static_cast<float>(advance_width) * m_scale, 0.0f);
                math::vec2  bearing(static_cast<float>(left_side_bearing) * m_scale, 0.0f);
                math::size2 size;

                int32 x0 = 0, y0 = 0, x1 = 0, y1 = 0;
                if (stbtt_GetGlyphBox(&m_impl->info, glyph_idx, &x0, &y0, &x1, &y1)) {
                    auto x0f = static_cast<float>(x0) * m_scale;
                    auto y0f = static_cast<float>(-y1) * m_scale;
                    auto x1f = static_cast<float>(x1) * m_scale;
                    auto y1f = static_cast<float>(-y0) * m_scale;
                    bearing.y = y1f;
                    size = math::size2(x1f - x0f, y1f - y0f);
                }

                m_glyphs.emplace_back(cp, atlas_entry{}, glyph_metrics{advance, bearing, size});
            }
        }

        m_is_init = true;
    }

    bool truetype_font::is_init() const noexcept
    {
        return m_is_init;
    }

    void truetype_font::shutdown() noexcept
    {
        if (m_is_init) {
            m_is_init = false;
            m_glyphs.clear();
            m_font_metrics = {0.0f, 0.0f, 0.0f};
            m_font_data = nullptr;
        }
    }

    math::isize2 truetype_font::glyph_bitmap_size(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix) const noexcept
    {
        auto        pad2 = static_cast<int32>(math::ceil(glyph_sdf_pad_pix)) * 2;
        const auto& g = get_glyph_info(idx);

        if (0 == idx) {
            auto sz = g.metrics.size * glyph_scale_pix;
            return math::ivec2(math::ceil(sz.width) + pad2, math::ceil(sz.height) + pad2);
        }

        int32 ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
        stbtt_GetCodepointBitmapBoxSubpixel(&m_impl->info, g.codepoint, m_scale * glyph_scale_pix, m_scale * glyph_scale_pix, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);

        int32 w = (ix1 - ix0) + pad2;
        int32 h = (iy1 - iy0) + pad2;

        return math::ivec2(w, h);
    }

    void truetype_font::bake_glyph_bitmap(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix, core::buffer_span<uint8> pixels, uint32 pixels_stride) const noexcept
    {
        const auto& g = get_glyph_info(idx);

        auto  pad = static_cast<int32>(math::ceil(glyph_sdf_pad_pix));
        auto  pix_dist_scale = pad == 0 ? 256.0f : 128.0f / static_cast<float>(pad);
        auto* dst = pixels.data();

        if (0 == idx) {
            auto sz = g.metrics.size * glyph_scale_pix;
            auto bitmap_sz = math::ivec2(math::ceil(sz.width) + pad * 2, math::ceil(sz.height) + pad * 2);

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

                    auto val = static_cast<uint8>(math::clamp(dist * pix_dist_scale, 0.0f, 255.0f));
                    dst[y * pixels_stride + x] = val;
                }
            }
            return;
        }

        int32 stbtt_glyph_idx = stbtt_FindGlyphIndex(&m_impl->info, g.codepoint);
        stbtt_MakeGlyphSDF(&m_impl->info, m_scale * glyph_scale_pix, stbtt_glyph_idx, pad, 128, pix_dist_scale, dst, pixels_stride, nullptr, nullptr, nullptr, nullptr);
    }

    float truetype_font::get_kerning_internal(char32 cp1, char32 cp2) const noexcept
    {
        return stbtt_GetCodepointKernAdvance(&m_impl->info, cp1, cp2) * m_scale;
    }

} // namespace tavros::text
