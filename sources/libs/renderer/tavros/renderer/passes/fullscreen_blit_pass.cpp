#include <tavros/renderer/render_pass/fullscreen_blit_pass.hpp>

namespace tavros::renderer
{
    fullscreen_blit_pass::fullscreen_blit_pass(render_system* rs)
        : m_rs(rs)
    {
        auto* rm = m_rs->resource_manager();
        rm->set_material_load_params({}, 1, rhi::pixel_format::none);
        m_mt = rm->load_material("mt.fullscreen_quad");
    }

    fullscreen_blit_pass::~fullscreen_blit_pass() noexcept
    {
        m_rs->resource_manager()->release_material(m_mt);
    }

    void fullscreen_blit_pass::upload(frame_context& ctx)
    {
    }

    void fullscreen_blit_pass::begin_pass(frame_context& ctx)
    {
        ctx.commands().push_debug_group("FullscreenBlitPass");
    }

    void fullscreen_blit_pass::execute(frame_context& ctx)
    {
    }

    void fullscreen_blit_pass::end_pass(frame_context& ctx)
    {
        ctx.commands().pop_debug_group();
    }

} // namespace tavros::renderer