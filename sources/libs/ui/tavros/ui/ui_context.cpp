#include <tavros/ui/ui_context.hpp>

#include <tavros/core/debug/assert.hpp>

namespace tavros::ui
{

    ui_context::ui_context(core::shared_ptr<renderer::render_system> rs) noexcept
        : m_renderer(std::move(rs))
    {
        TAV_ASSERT(m_renderer);
    }

    renderer::render_system& ui_context::renderer() const noexcept
    {
        return *m_renderer;
    }

    void ui_context::set_debug_draw_enabled(bool enabled) noexcept
    {
        m_debug_info.debug_draw = enabled;
    }

    const debug_draw_info& ui_context::debug_info() const noexcept
    {
        return m_debug_info;
    }

} // namespace tavros::ui
