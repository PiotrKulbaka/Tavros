#pragma once
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/gpu_stage_buffer.hpp>
#include <tavros/core/containers/vector.hpp>

namespace tavros::renderer
{

    class upload_context : core::noncopyable
    {
    public:
        static constexpr auto k_stage_buffer_size = 64_mib;

        struct upload_batch
        {
            gpu_buffer_view<uint8> view;
            rhi::command_queue*    queue;
        };

    public:
        explicit upload_context(rhi::graphics_device* gdevice) noexcept;
        ~upload_context() noexcept;

        void flush() noexcept;

        [[nodiscard]] upload_batch slice(size_t size) noexcept;

    private:
        rhi::graphics_device* m_gdevice = nullptr;
        rhi::command_queue*   m_current_upload_queue = nullptr;
        rhi::fence_handle     m_fence;
        gpu_stage_buffer      m_stage_buffer;
        gpu_stage_buffer      m_large_stage_buffer;
    };

} // namespace tavros::renderer