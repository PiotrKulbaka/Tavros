#pragma once

#include <tavros/renderer/render_pass/render_pass.hpp>

namespace tavros::renderer
{

    class forward_opaque_pass final : public render_pass
    {
    public:
        forward_opaque_pass(render_system* rs);
        ~forward_opaque_pass() noexcept override;

        void upload(frame_context& ctx) override;
        void begin_pass(frame_context& ctx) override;
        void execute(frame_context& ctx) override;
        void end_pass(frame_context& ctx) override;

    public:
    };

} // namespace tavros::renderer