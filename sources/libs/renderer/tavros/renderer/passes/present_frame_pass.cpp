#include <tavros/renderer/passes/present_frame_pass.hpp>

#include <glad/glad.h>

namespace tavros::renderer
{

    present_frame_pass::present_frame_pass(render_system* rs)
        : m_rs(rs)
    {
        TAV_ASSERT(m_rs);
        m_mt = m_rs->resource_manager()->load_material("mt.fullscreen_quad");
    }

    present_frame_pass::~present_frame_pass()
    {
        m_rs->resource_manager()->release_material(m_mt);
    }

    void present_frame_pass::upload(frame_context& ctx)
    {
    }

    void present_frame_pass::execute(frame_context& ctx)
    {
        const auto backbuffer = ctx.frame_data().get_data<fd_backbuffer>();
        const auto attachment = ctx.frame_data().get_data<fd_base_color_ca>();

        // Ensure any previous rendering is finished before presenting
        ctx.commands().end_rendering();

        ctx.commands().push_debug_group("Present");

        ctx.commands().begin_rendering(backbuffer.fb);
        ctx.commands().bind_pipeline(m_mt->gpu_pipeline());
        auto smp = ctx.resource_manager().sampler(sampler_preset::linear_clamp);
        ctx.commands().bind_shader_textures(rhi::texture_binding{attachment.tex, smp, 0});
        ctx.commands().draw(4);
        ctx.commands().end_rendering();

        ctx.commands().pop_debug_group();
    }

} // namespace tavros::renderer
