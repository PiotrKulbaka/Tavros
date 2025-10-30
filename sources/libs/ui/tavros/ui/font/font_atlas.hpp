#pragma once

#include <tavros/ui/base.hpp>
#include <tavros/ui/font/truetype_font.hpp>

namespace tavros::ui
{

    class font_atlas : core::noncopyable
    {
    public:
        struct glyph_info
        {
            uint32                       serial_index = 0; // пор€дковый номер глифа в мапе
            truetype_font*               font = nullptr;
            uint32                       codepoint = 0;
            truetype_font::glyph_index   glyph = 0;
            float                        glyph_scale = 0.0f; // in pixels
            float                        sdf_padding = 0.0f; // in pixels
            vec2                         uv1;
            vec2                         uv2;
            int32                        left = 0;
            int32                        top = 0;
            int32                        right = 0;
            int32                        bottom = 0;
            truetype_font::glyph_metrics metrics;
        };

        struct atlas_pixels
        {
            uint8* pixels; // ”казатель на grayscale пиксели
            uint32 width;
            uint32 height;
            uint32 stride;
        };

    public:
        font_atlas();
        ~font_atlas();

        void begin_atlas();
        bool add_glyph_from(truetype_font* font, uint32 codepoint, float scale, float sdf_padding);
        void end_atlas();

        isize2 get_atlas_size() const;

        bool bake_atlas(atlas_pixels atlas);

        core::unordered_map<uint32, glyph_info>& map()
        {
            return m_glyph_map;
        }

    private:
        core::unordered_map<uint32, glyph_info> m_glyph_map;
        isize2                                  m_atlas_size;
    };

} // namespace tavros::ui
