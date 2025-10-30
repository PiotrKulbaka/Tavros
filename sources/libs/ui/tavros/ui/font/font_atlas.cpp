#include <tavros/ui/font/font_atlas.hpp>

namespace tavros::ui
{

    font_atlas::font_atlas()
    {
    }

    font_atlas::~font_atlas()
    {
    }

    void font_atlas::begin_atlas()
    {
    }

    bool font_atlas::add_glyph_from(truetype_font* font, uint32 codepoint, float scale, float sdf_padding)
    {
        auto glyph = font->get_glyph_index(codepoint);
        if (0 == glyph && codepoint != 0) {
            return false;
        }

        glyph_info gi;
        gi.font = font;
        gi.codepoint = codepoint;
        gi.glyph = glyph;
        gi.glyph_scale = scale;
        gi.sdf_padding = sdf_padding;

        m_glyph_map[codepoint] = gi;
        return true;
    }

    void font_atlas::end_atlas()
    {
        constexpr int32 pixels_width = 2048;

        int32 left = 1;
        int32 top = 1;
        int32 max_row_h = 0;

        for (auto& item : m_glyph_map) {
            auto& info = item.second;
            info.metrics = info.font->get_glyph_metrics(info.glyph);

            int32 pad = static_cast<int32>(math::ceil(info.sdf_padding));
            int32 w = static_cast<int32>(math::ceil(info.metrics.width * info.glyph_scale)) + pad * 2;
            int32 h = static_cast<int32>(math::ceil(info.metrics.height * info.glyph_scale)) + pad * 2;

            if (left + w > pixels_width) {
                left = 1;
                top += max_row_h + 2;
                max_row_h = 0;
            }

            if (max_row_h < h) {
                max_row_h = h;
            }

            info.left = left;
            info.top = top;
            info.right = left + w;
            info.bottom = top + h;

            left += w + 2;
        }

        int32 min_pixels_height = top + max_row_h + 2;
        m_atlas_size = {pixels_width, min_pixels_height};
    }

    isize2 font_atlas::get_atlas_size() const
    {
        return m_atlas_size;
    }

    bool font_atlas::bake_atlas(atlas_pixels atlas)
    {
        if (0 == m_atlas_size.width || 0 == m_atlas_size.height) {
            return false;
        }

        auto w = static_cast<float>(atlas.width);
        auto h = static_cast<float>(atlas.height);

        size_t im_size = static_cast<size_t>(atlas.width) * static_cast<size_t>(atlas.stride);

        uint32 serial_index = 0;
        for (auto& item : m_glyph_map) {
            auto&       info = item.second;
            const float subpix = 0.005f;

            vec2 min((static_cast<float>(info.left) + subpix) / w, (static_cast<float>(info.top) + subpix) / h);
            vec2 max((static_cast<float>(info.right) - subpix) / w, (static_cast<float>(info.bottom) - subpix) / h);
            info.uv1 = min;
            info.uv2 = max;
            info.serial_index = serial_index++;

            size_t offset = static_cast<size_t>(info.top) * static_cast<size_t>(atlas.stride) + static_cast<size_t>(info.left);
            uint8* dst = atlas.pixels + offset;
            info.font->make_glyph_sdf(info.glyph, {dst, im_size - offset}, atlas.stride, info.glyph_scale, info.sdf_padding);
        }

        return true;
    }

} // namespace tavros::ui
