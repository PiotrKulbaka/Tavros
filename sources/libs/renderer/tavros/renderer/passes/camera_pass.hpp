#pragma once

#include <tavros/renderer/frame/render_pass.hpp>

namespace tavros::renderer
{

    class camera_pass final : public render_pass
    {
    public:
        camera_pass(render_system* rs);
        ~camera_pass() noexcept override;

        void upload(frame_context& ctx) override;
        void execute(frame_context& ctx) override;

    private:
        rhi::buffer_binding m_camera_binding;
    };

} // namespace tavros::renderer