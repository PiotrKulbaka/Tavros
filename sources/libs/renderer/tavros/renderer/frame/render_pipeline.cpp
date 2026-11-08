#include <tavros/renderer/frame/render_pipeline.hpp>

namespace tavros::renderer
{

    render_pipeline::render_pipeline(core::vector<core::unique_ptr<render_pass>>&& passes)
        : m_passes(std::move(passes))
    {
    }

    render_pipeline::~render_pipeline() noexcept = default;

    void render_pipeline::run(frame_context& ctx)
    {
        ctx.commands().push_debug_group("Upload");
        for (auto& pass : m_passes) {
            pass->upload(ctx);
        }
        ctx.commands().pop_debug_group();

        ctx.commands().push_debug_group("Execute");
        for (auto& pass : m_passes) {
            pass->execute(ctx);
        }
        ctx.commands().pop_debug_group();
    }

} // namespace tavros::renderer
