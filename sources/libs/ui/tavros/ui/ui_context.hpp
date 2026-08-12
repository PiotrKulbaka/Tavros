#pragma once

#include <tavros/renderer/render_system.hpp>

namespace tavros::ui
{

    struct debug_draw_info
    {
        bool          debug_draw = true;
        math::color32 content_box_color = math::color32(0x60FFC97F);
        math::color32 padding_box_color = math::color32(0x60C5FF7F);
        math::color32 border_box_color = math::color32(0x60ED7FFF);
        math::color32 margin_box_color = math::color32(0x607FE9FF);
        math::color32 hit_box_color = math::color32(0xFF8EFF7F);
        float         hit_box_line_thickness = 2.0f;
    };

    class ui_context : core::noncopyable
    {
    public:
        ui_context(core::shared_ptr<renderer::render_system> rs) noexcept;
        ~ui_context() noexcept = default;

        renderer::render_system& renderer() const noexcept;

        void set_debug_draw_enabled(bool enabled) noexcept;

        const debug_draw_info& debug_info() const noexcept;

    private:
        core::shared_ptr<renderer::render_system> m_renderer;
        debug_draw_info                           m_debug_info;
    };

} // namespace tavros::ui