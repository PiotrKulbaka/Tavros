#pragma once

#include <tavros/renderer/frame/render_pass.hpp>

namespace tavros::renderer
{

    class ui_render_pass final : public render_pass
    {
    public:
        ui_render_pass(render_system* rs);
        ~ui_render_pass() noexcept override;

        void upload(frame_context& ctx) override;
        void execute(frame_context& ctx) override;

    private:
        render_system* m_rs = nullptr;
        material_ref   m_ui_mt;

        rhi::buffer_binding m_draw_buffer_view;
        rhi::buffer_binding m_paint_buffer_view;
    };

} // namespace tavros::renderer