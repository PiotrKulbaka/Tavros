#pragma once

#include <tavros/renderer/frame/render_pass.hpp>

namespace tavros::renderer
{

    class forward_attachments_pass final : public render_pass
    {
    public:
        forward_attachments_pass(render_system* rs);
        ~forward_attachments_pass() noexcept override;

        void upload(frame_context& ctx) override;
        void execute(frame_context& ctx) override;

    private:
        render_system*    m_rs = nullptr;
        uint32            m_current_msaa = 0;
        uint32            m_current_width = 0;
        uint32            m_current_height = 0;
        render_target_ref m_rt;
    };

} // namespace tavros::renderer