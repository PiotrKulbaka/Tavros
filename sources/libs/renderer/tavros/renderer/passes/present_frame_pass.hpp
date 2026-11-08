#pragma once

#include <tavros/renderer/frame/render_pass.hpp>
#include <tavros/renderer/render_system.hpp>

namespace tavros::renderer
{

    class present_frame_pass : public render_pass
    {
    public:
        present_frame_pass(render_system* rs);
        ~present_frame_pass() noexcept override;

        void upload(frame_context& ctx) override;
        void execute(frame_context& ctx) override;

    private:
        render_system* m_rs;
        material_ref   m_mt;
    };

} // namespace tavros::renderer
