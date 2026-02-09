#include <tavros/text/font/font_atlas.hpp>

#include <tavros/core/math/bitops.hpp>
#include <tavros/core/logger/logger.hpp>

#include <stb/stb_rect_pack.h>

namespace
{
    tavros::core::logger logger("font_atlas");
}

namespace tavros::text
{

    font_atlas::font_atlas()
        : m_need_to_recreate(false)
    {
    }

    font_atlas::~font_atlas()
    {
    }

    void font_atlas::register_font(font* fnt) noexcept
    {
        m_fonts.push_back(fnt);
        m_need_to_recreate = true;
    }

    bool font_atlas::need_to_recreate_atlas() const noexcept
    {
        return m_need_to_recreate;
    }

    font_atlas::atlas_pixels font_atlas::invalidate_old_and_bake_new_atlas(float glyph_scale_pix, float glyph_sdf_pad_pix)
    {
        // 1. The first step is packing the rectangles

        // Calc number of rects
        size_t total_glyphs = 0;
        for (auto* fnt : m_fonts) {
            total_glyphs += fnt->m_glyphs.size();
        }

        if (0 == total_glyphs) {
            ::logger.error("Atlas is empty");
            return {};
        }

        // Init rects
        core::vector<stbrp_rect> rects;
        rects.reserve(total_glyphs);

        int32 id = 0;
        for (auto* fnt : m_fonts) {
            auto num_glyphs = static_cast<font::glyph_index>(fnt->m_glyphs.size());
            for (font::glyph_index glyph_idx = 0; glyph_idx < num_glyphs; ++glyph_idx) {
                auto bitmap_size = fnt->glyph_bitmap_size(glyph_idx, glyph_scale_pix, glyph_sdf_pad_pix);
                rects.emplace_back(id++, bitmap_size.width + 1, bitmap_size.height + 1, 0, 0, 0);
            }
        }


        uint32 one_glyph_max_size = static_cast<uint32>(math::ceil(glyph_scale_pix) + math::ceil(glyph_sdf_pad_pix) * 2.0f);
        uint32 pack_side_size = static_cast<uint32>(math::floor_power_of_two(static_cast<uint64>(std::sqrt(static_cast<double>(total_glyphs))) * one_glyph_max_size));

        // Packing context
        stbrp_context            ctx;
        core::vector<stbrp_node> nodes;

        // Also reserve for the next packaging attempt
        nodes.reserve(pack_side_size * 2);

        while (true) {
            if (pack_side_size >= 32768) {
                ::logger.error("The atlas size is too big");
                return {};
            }

            // Does nothing the first time
            nodes.reserve(pack_side_size);

            auto w = static_cast<int32>(pack_side_size) - 1;
            auto h = static_cast<int32>(pack_side_size) - 1;
            stbrp_init_target(&ctx, w, h, nodes.data(), w);
            auto num_rects = static_cast<int32>(rects.size());
            if (stbrp_pack_rects(&ctx, rects.data(), num_rects)) {
                // All rects was packed. Go to next step
                break;
            }

            // Try packing the rectangles again with a larger size
            pack_side_size *= 2;
        }


        // 2. The second step is rendering the glyphs into the atlas

        // Calculate the max height of the atlas
        uint32 atlas_width = pack_side_size;
        uint32 atlas_height = 0;
        for (auto& r : rects) {
            auto height_candidate = static_cast<uint32>(r.y + r.h);
            if (height_candidate > atlas_height) {
                atlas_height = height_candidate;
            }
        }


        auto                atlas_size = static_cast<size_t>(atlas_width) * static_cast<size_t>(atlas_height);
        auto                stride = atlas_width;
        core::vector<uint8> pixels(atlas_size, 0);

        // Bake atlas
        size_t rect_idx = 0;
        for (auto* fnt : m_fonts) {
            auto num_glyphs = static_cast<font::glyph_index>(fnt->m_glyphs.size());
            for (font::glyph_index glyph_idx = 0; glyph_idx < num_glyphs; ++glyph_idx) {
                auto& rect = rects[rect_idx++];
                auto& g = fnt->m_glyphs[glyph_idx];

                g.entry.left = static_cast<uint16>(rect.x + 1);
                g.entry.top = static_cast<uint16>(rect.y + 1);
                g.entry.right = static_cast<uint16>(rect.x + rect.w);
                g.entry.bottom = static_cast<uint16>(rect.y + rect.h);

                // Bake glyph
                auto offset = static_cast<size_t>(rect.y + 1) * static_cast<size_t>(stride) + static_cast<size_t>(rect.x + 1);
                auto dst = core::buffer_span<uint8>(pixels.data() + offset, atlas_size - offset);

                fnt->bake_glyph_bitmap(glyph_idx, glyph_scale_pix, glyph_sdf_pad_pix, dst, atlas_width);
            }
        }

        return {pixels, atlas_width, atlas_height, stride};
    }

} // namespace tavros::text
