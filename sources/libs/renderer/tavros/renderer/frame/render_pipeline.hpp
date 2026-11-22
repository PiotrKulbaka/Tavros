#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/renderer/frame/frame_context.hpp>
#include <tavros/renderer/frame/render_pass.hpp>

namespace tavros::renderer
{

    class frame_graph;

    class render_pipeline : core::noncopyable
    {
    public:
        render_pipeline(core::vector<core::unique_ptr<render_pass>>&& passes);

        ~render_pipeline() noexcept;

        void run(frame_context& ctx);

    public:
        core::vector<core::unique_ptr<render_pass>> m_passes;
    };

} // namespace tavros::renderer
