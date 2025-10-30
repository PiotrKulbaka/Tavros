#include <tavros/ui/font/truetype_font.hpp>

#include <tavros/core/logger/logger.hpp>
#include <stb/stb_truetype.h>

namespace
{
    tavros::core::logger logger("truetype_font");
}

namespace tavros::ui
{

    struct truetype_font::impl
    {
        stbtt_fontinfo info;
    };

    truetype_font::truetype_font()
        : m_data(nullptr)
        , m_is_init(false)
    {
        m_glyph_metrics.reserve(128);
    }

    truetype_font::~truetype_font()
    {
        shutdown();
    }

    bool truetype_font::init(core::dynamic_buffer<uint8> font_data)
    {
        if (stbtt_InitFont(&m_impl->info, font_data.data(), 0) == 0) {
            ::logger.error("Failed to init font data");
            m_is_init = false;
            return false;
        }

        m_data = std::move(font_data);

        int32 ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&m_impl->info, &ascent, &descent, &line_gap);
        m_scale = stbtt_ScaleForPixelHeight(&m_impl->info, 1.0f);

        m_font_metrics.ascent_y = static_cast<float>(ascent) * m_scale;
        m_font_metrics.descent_y = static_cast<float>(descent) * m_scale;
        m_font_metrics.line_gap_y = static_cast<float>(line_gap) * m_scale;

        m_is_init = true;
        return true;
    }

    void truetype_font::shutdown()
    {
        if (m_is_init) {
            m_is_init = false;
            m_glyph_metrics.clear();
            m_font_metrics = {};
        }
    }

    bool truetype_font::is_init()
    {
        return m_is_init;
    }

    truetype_font::font_metrics truetype_font::get_font_metrics()
    {
        TAV_ASSERT(m_is_init);

        return m_font_metrics;
    }

    truetype_font::glyph_index truetype_font::get_glyph_index(uint32 codepoint)
    {
        int32 glyph = codepoint == 0 ? 0 : stbtt_FindGlyphIndex(&m_impl->info, codepoint);

        glyph_index idx = static_cast<glyph_index>(glyph);

        if (idx != 0 || codepoint == 0) {
            if (auto it = m_glyph_metrics.find(idx); it == m_glyph_metrics.end()) {
                // preinitialize
                glyph_metrics m;
                m.codepoint = codepoint;
                m_glyph_metrics[idx] = m;
            }
        }

        return idx;
    }

    truetype_font::glyph_metrics truetype_font::get_glyph_metrics(glyph_index glyph)
    {
        TAV_ASSERT(m_is_init);

        if (auto it = m_glyph_metrics.find(glyph); it != m_glyph_metrics.end()) {
            auto& m = it->second;

            if (m.width == 0.0f || m.height == 0.0f) {
                // Metrics was initialized but not filled
                int32 advance_width = 0, left_side_bearing = 0;
                stbtt_GetGlyphHMetrics(&m_impl->info, glyph, &advance_width, &left_side_bearing);

                float x0f = 0.0f, y0f = 0.0f, x1f = 0.0f, y1f = 0.0f;
                int32 x0 = 0, y0 = 0, x1 = 0, y1 = 0;
                if (stbtt_GetGlyphBox(&m_impl->info, glyph, &x0, &y0, &x1, &y1)) {
                    x0f = static_cast<float>(x0) * m_scale;
                    y0f = static_cast<float>(-y1) * m_scale;
                    x1f = static_cast<float>(x1) * m_scale;
                    y1f = static_cast<float>(-y0) * m_scale;
                }

                m.advance_x = static_cast<float>(advance_width) * m_scale;
                m.advance_y = 0.0f;
                m.bearing_x = static_cast<float>(left_side_bearing) * m_scale;
                m.bearing_y = y1f;
                m.width = x1f - x0f;
                m.height = y1f - y0f;
            }

            return m;
        }

        return {};
    }

    float truetype_font::get_kerning(glyph_index glyph_left, glyph_index glyph_right)
    {
        TAV_ASSERT(m_is_init);
        int32 kern_advance = stbtt_GetGlyphKernAdvance(&m_impl->info, static_cast<int32>(glyph_left), static_cast<int32>(glyph_right));
        return static_cast<float>(kern_advance) * m_scale;
    }

    bool truetype_font::make_glyph_sdf(glyph_index glyph, tavros::core::buffer_span<uint8> pixels, uint32 stride, float font_height, float sdf_pad)
    {
        if (!m_is_init) {
            return false;
        }

        auto  pad = static_cast<int32>(math::ceil(sdf_pad));
        auto  pix_dist_scale = pad == 0 ? 256.0f : 128.0f / static_cast<float>(pad);
        auto* dst = pixels.data();

        return !!stbtt_MakeGlyphSDF(&m_impl->info, m_scale * font_height, glyph, pad, 128, pix_dist_scale, dst, stride, nullptr, nullptr, nullptr, nullptr);
    }

} // namespace tavros::ui
