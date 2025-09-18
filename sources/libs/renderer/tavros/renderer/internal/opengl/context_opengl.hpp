#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/renderer/rhi/frame_composer_info.hpp>

namespace tavros::renderer::rhi
{

    class context_opengl
    {
    public:
        static core::unique_ptr<context_opengl> create(const frame_composer_info& info, void* native_handle);

    public:
        virtual ~context_opengl() = default;

        virtual void make_current() = 0;

        virtual void make_inactive() = 0;

        virtual void swap_buffers() = 0;

        virtual bool is_current() = 0;
    };

} // namespace tavros::renderer::rhi
