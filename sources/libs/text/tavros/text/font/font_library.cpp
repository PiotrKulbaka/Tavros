#include <tavros/text/font/font_library.hpp>

#include <tavros/core/exception.hpp>
#include <tavros/text/font/truetype_font.hpp>

namespace tavros::text
{

    font_library::font_library(core::unique_ptr<font_data_provider> dp) noexcept
        : m_data_provider(std::move(dp))
    {
        m_vector.reserve(32);
    }

    void font_library::load(core::string_view path, core::string_view name)
    {
        if (auto it = m_fonts.find(name); it != m_fonts.end()) {
            throw std::runtime_error("Font with name '" + core::string(name) + "' already loaded");
        }

        auto data = m_data_provider->load(path);

        constexpr truetype_font::codepoint_range ranges[] = {
            {0x0, 0x7f},
            {0xab, 0xbb},
            {0x401, 0x401},   // ru ¨
            {0x410, 0x44F},   // ru À–ßà–ÿ
            {0x451, 0x451},   // ru ¸
            {0x2012, 0x2014}, // dash
            {0x2022, 0x2026}, // circle, riangle, one dot, two dot, three dot
            {0xfffd, 0xfffd}, // replacement character
        };

        core::unique_ptr<font> fnt = core::make_unique<truetype_font>(std::move(data), ranges);
        font*                  p = fnt.get();

        auto result = m_fonts.try_emplace(core::string(name), std::move(fnt));
        m_vector.emplace_back(font_view{result.first->first, p});
        m_atlas.register_font(p);
    }

    font* font_library::try_get(core::string_view name) const noexcept
    {
        if (auto it = m_fonts.find(name); it != m_fonts.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool font_library::need_to_recreate_atlas() const noexcept
    {
        return m_atlas.need_to_recreate_atlas();
    }

    font_atlas::atlas_pixels font_library::invalidate_old_and_bake_new_atlas(float glyph_scale_pix, float glyph_sdf_pad_pix)
    {
        return m_atlas.invalidate_old_and_bake_new_atlas(glyph_scale_pix, glyph_sdf_pad_pix);
    }

    core::buffer_view<font_library::font_view> font_library::fonts() const noexcept
    {
        return m_vector;
    }

} // namespace tavros::text