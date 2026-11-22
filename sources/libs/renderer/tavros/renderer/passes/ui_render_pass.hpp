#pragma once

#include <tavros/renderer/frame/render_pass.hpp>

namespace tavros::renderer
{

    class ui_render_pass final : public render_pass
    {
    public:
        struct draw_command_t;
        struct ui_batch_textures;
        struct ui_batch;
        struct ui_cmd_entry;

    public:
        ui_render_pass(render_system* rs);
        ~ui_render_pass() noexcept override;

        void upload(frame_context& ctx) override;
        void execute(frame_context& ctx) override;

    private:
        render_system* m_rs = nullptr;
        material_ref   m_ui_mt;

        core::vector<draw_command_t> m_draw_commands;
        core::vector<ui_batch>       m_batches;
        core::vector<ui_cmd_entry>   m_entries;
        rhi::buffer_binding          m_draw_buffer_view;
        rhi::buffer_binding          m_paint_buffer_view;
        rhi::texture_handle          m_white_tex;
        rhi::texture_handle          m_debug_tex;
    };

} // namespace tavros::renderer