#pragma once

#include <tavros/renderer/render_system.hpp>

namespace tavros::ui
{

    struct debug_draw_info
    {
        bool debug_draw = true;
        math::rgba8 content_box_color = math::rgba8(0x7FC9FF60);
        math::rgba8 padding_box_color = math::rgba8(0x7FFFC560);
        math::rgba8 border_box_color = math::rgba8(0xFF7FED60);
        math::rgba8 margin_box_color = math::rgba8(0xFFE97F60);
        math::rgba8 hit_box_color = math::rgba8(0x7FFF8EFF);
        float       hit_box_line_thickness = 2.0f;
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
        debug_draw_info m_debug_info;
    };

} // namespace tavros::ui