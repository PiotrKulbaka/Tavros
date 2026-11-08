#pragma once

#include <tavros/renderer/frame/frame_context.hpp>

namespace tavros::renderer
{

    class render_pass
    {
    public:
        virtual ~render_pass() noexcept = default;

        virtual void upload(frame_context& ctx) = 0;
        virtual void execute(frame_context& ctx) = 0;
    };

} // namespace tavros::renderer
