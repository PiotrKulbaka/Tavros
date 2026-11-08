#pragma once

#include <tavros/renderer/render_pass/render_pass.hpp>
#include <tavros/renderer/render_system.hpp>

namespace tavros::renderer
{

    class fullscreen_blit_pass final : public render_pass
    {
    public:
        fullscreen_blit_pass(render_system* rs);
        ~fullscreen_blit_pass() noexcept override;

        void upload(frame_context& ctx) override;
        void begin_pass(frame_context& ctx) override;
        void execute(frame_context& ctx) override;
        void end_pass(frame_context& ctx) override;

    public:
        render_system* m_rs;
        material_ref   m_mt;
    };

} // namespace tavros::renderer