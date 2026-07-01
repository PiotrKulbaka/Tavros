#include <tavros/renderer/text/font/truetype_font.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/defines.hpp>
#include <tavros/core/math/functions/clamp.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/core/memory/memory.hpp>

#include <stb/stb_truetype.h>

namespace
{
    tavros::core::logger logger("truetype_font");

    using cp_rng = tavros::renderer::font_desc::codepoint_range;


    tavros::core::vector<cp_rng> sort_and_merge_codepoint_ranges(tavros::core::buffer_view<cp_rng> ranges)
    {
        tavros::core::vector<cp_rng> sorted_ranges(ranges.begin(), ranges.end());

        std::sort(sorted_ranges.begin(), sorted_ranges.end(), [](const cp_rng& lhs, const cp_rng& rhs) noexcept {
            return lhs.first_codepoint < rhs.first_codepoint;
        });

        size_t dst = 0;

        for (size_t src = 0; src < sorted_ranges.size(); ++src) {
            const auto range = sorted_ranges[src];

            // Skip invalid ranges.
            if (range.first_codepoint > range.last_codepoint) {
                continue;
            }

            if (dst == 0) {
                sorted_ranges[dst++] = range;
                continue;
            }

            auto& last = sorted_ranges[dst - 1];

            // Merge overlapping and adjacent ranges.
            if (range.first_codepoint <= last.last_codepoint + 1) {
                last.last_codepoint = std::max(last.last_codepoint, range.last_codepoint);
            } else {
                sorted_ranges[dst++] = range;
            }
        }

        sorted_ranges.resize(dst);

        return sorted_ranges;
    }
} // namespace

namespace tavros::renderer
{

    struct truetype_font::impl
    {
        stbtt_fontinfo info;
    };

    truetype_font::truetype_font(font_atlas* atlas, core::dynamic_buffer<uint8> font_data, core::buffer_view<font_desc::codepoint_range> codepoint_ranges)
        : font()
        , m_font_data(std::move(font_data))
        , m_scale(0.0f)
        , m_atlas(atlas)
    {
        m_impl = core::make_unique<impl>();

        if (stbtt_InitFont(&m_impl->info, m_font_data.data(), 0) == 0) {
            throw core::format_error(core::format_error_tag::invalid_data, "Failed to init font data");
        }

        int32 ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&m_impl->info, &ascent, &descent, &line_gap);
        m_scale = stbtt_ScaleForPixelHeight(&m_impl->info, 1.0f);

        m_font_metrics.ascent_y = static_cast<float>(ascent) * m_scale;
        m_font_metrics.descent_y = static_cast<float>(descent) * m_scale;
        m_font_metrics.line_gap_y = static_cast<float>(line_gap) * m_scale;
        m_font_metrics.sdf_padding_pix = 0.0f; // Set externally during calling 'invalidate_old_and_bake_new_atlas' method in 'font_atlas' class

        tavros::core::vector<cp_rng> ranges;
        if (codepoint_ranges.empty()) {
            ranges.emplace_back(cp_rng{0x20, 0x7E}); // Default to ASCII range if no ranges are provided
        } else {
            ranges = sort_and_merge_codepoint_ranges(codepoint_ranges);
        }

        for (const auto& range : ranges) {
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

                m_glyphs.emplace_back(cp, atlas_rect_t{}, glyph_metrics{advance, bearing, size});
            }
        }

        m_atlas->register_font(this);
    }

    truetype_font::~truetype_font() noexcept
    {
        m_atlas->unreg_font(this);
        m_impl = nullptr;
    }

    math::isize2 truetype_font::glyph_bitmap_size_internal(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix) const noexcept
    {
        auto        pad2 = static_cast<int32>(math::ceil(glyph_sdf_pad_pix)) * 2;
        const auto& g = get_glyph_info(idx);

        int32 ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
        stbtt_GetCodepointBitmapBoxSubpixel(&m_impl->info, g.codepoint, m_scale * glyph_scale_pix, m_scale * glyph_scale_pix, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);

        int32 w = (ix1 - ix0) + pad2;
        int32 h = (iy1 - iy0) + pad2;

        return math::ivec2(w, h);
    }

    void truetype_font::bake_glyph_bitmap_internal(glyph_index idx, float glyph_scale_pix, float glyph_sdf_pad_pix, core::buffer_span<uint8> pixels, uint32 pixels_stride) const noexcept
    {
        const auto& g = get_glyph_info(idx);

        auto pad = static_cast<int32>(math::ceil(glyph_sdf_pad_pix));
        auto pix_dist_scale = pad == 0 ? 256.0f : 128.0f / static_cast<float>(pad);

        int32 stbtt_glyph_idx = stbtt_FindGlyphIndex(&m_impl->info, g.codepoint);
        stbtt_MakeGlyphSDF(&m_impl->info, m_scale * glyph_scale_pix, stbtt_glyph_idx, pad, 128, pix_dist_scale, pixels.data(), pixels_stride, nullptr, nullptr, nullptr, nullptr);
    }

    float truetype_font::get_kerning_internal(char32 cp1, char32 cp2) const noexcept
    {
        return stbtt_GetCodepointKernAdvance(&m_impl->info, cp1, cp2) * m_scale;
    }

} // namespace tavros::renderer
