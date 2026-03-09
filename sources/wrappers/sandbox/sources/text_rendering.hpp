#pragma once

#include <tavros/core/archetype.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/rgba8.hpp>
#include <tavros/renderer/text/text_builder.hpp>
#include <tavros/renderer/text/text_layouter.hpp>
#include <tavros/renderer/text/glyph_data.hpp>

namespace app
{

    // -------------------------------------------------------------------------
    // GPU glyph instance (matches sdf_font.vert attribute layout)
    // -------------------------------------------------------------------------

    struct glyph_instance
    {
        float                          mat[3][2] = {};
        tavros::renderer::atlas_rect_t rect;
        tavros::math::rgba8            fill_color;
        tavros::math::rgba8            outline_color;
    };

    // -------------------------------------------------------------------------
    // Text archetype
    // -------------------------------------------------------------------------

    using text_archetype = tavros::core::basic_archetype<
        tavros::renderer::glyph_c,
        tavros::renderer::atlas_rect_t,
        tavros::renderer::rect_layout_c,
        tavros::renderer::pos2_c,
        tavros::renderer::primary_color_c,
        tavros::renderer::outline_color_c>;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Fills @p buf with GPU glyph instance data from @p data.
     * @return Number of glyphs written.
     */
    template<tavros::core::archetype_with<
        tavros::renderer::atlas_rect_t,
        tavros::renderer::rect_layout_c,
        tavros::renderer::pos2_c,
        tavros::renderer::primary_color_c,
        tavros::renderer::outline_color_c>
                 T>
    size_t fill_glyph_instances(
        T&                                        data,
        tavros::core::buffer_span<glyph_instance> buf,
        tavros::math::vec2                        pos_text
    )
    {
        TAV_ASSERT(buf.size() >= data.size());

        size_t len = 0;
        auto*  dst = buf.begin();

        data.view<
                const tavros::renderer::atlas_rect_t,
                const tavros::renderer::rect_layout_c,
                const tavros::renderer::pos2_c,
                const tavros::renderer::primary_color_c,
                const tavros::renderer::outline_color_c>()
            .each([&](const auto& r, const auto& l, const auto& p, const auto& pc, const auto& oc) {
                const auto size = l.size();

                dst->mat[0][0] = size.width;
                dst->mat[0][1] = 0.0f;
                dst->mat[1][0] = 0.0f;
                dst->mat[1][1] = size.height;
                dst->mat[2][0] = pos_text.x + p.x + l.left;
                dst->mat[2][1] = pos_text.y + p.y + l.top;
                dst->rect = r;
                dst->fill_color = pc;
                dst->outline_color = oc;

                ++dst;
                ++len;
            });

        return len;
    }

    /**
     * @brief Sets fill and outline colors for a range of glyphs in @p data.
     */
    template<tavros::core::archetype_with<
        tavros::renderer::primary_color_c,
        tavros::renderer::outline_color_c>
                 T>
    void set_glyph_colors(
        T&                 data,
        size_t             first,
        size_t             count,
        tavros::math::vec4 primary,
        tavros::math::vec4 outline
    ) noexcept
    {
        const tavros::renderer::primary_color_c pc{tavros::math::rgba8{primary}};
        const tavros::renderer::outline_color_c oc{tavros::math::rgba8{outline}};

        data.view<tavros::renderer::primary_color_c, tavros::renderer::outline_color_c>()
            .each_n(first, count, [&](auto& p, auto& o) {
                p = pc;
                o = oc;
            });
    }

    /**
     * @brief Appends colored text to @p text using @p fnt at @p font_size.
     */
    template<tavros::core::archetype_with<
        tavros::renderer::glyph_c,
        tavros::renderer::atlas_rect_t,
        tavros::renderer::rect_layout_c,
        tavros::renderer::primary_color_c,
        tavros::renderer::outline_color_c>
                 Text>
    void append_colored_text(
        Text&                         text,
        tavros::core::string_view     str,
        const tavros::renderer::font* fnt,
        float                         font_size,
        tavros::math::vec4            primary,
        tavros::math::vec4            outline
    ) noexcept
    {
        const auto first = text.size();
        tavros::renderer::text_builder::append_text(text, str, fnt, font_size);
        set_glyph_colors(text, first, text.size() - first, primary, outline);
    }

} // namespace app
