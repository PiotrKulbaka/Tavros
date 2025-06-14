#pragma once

#include <tavros/core/types.hpp>
#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer
{

    class swapchain
    {
    public:
        virtual ~swapchain() = default;

        virtual uint32 acquire_next_backbuffer_index() noexcept = 0;

        virtual framebuffer_handle get_framebuffer(uint32 backbuffer_index) = 0;

        virtual void present(uint32 backbuffer_index) = 0;

        virtual void resize(uint32 width, uint32 height) = 0;

        virtual uint32 width() const noexcept = 0;

        virtual uint32 height() const noexcept = 0;
    };

} // namespace tavros::renderer
