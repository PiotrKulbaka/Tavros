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
        upload_context(rhi::graphics_device* gdevice) noexcept;

        ~upload_context() noexcept;

        template<class T>
        [[nodiscard]] gpu_buffer_view<T> slice(size_t count) noexcept
        {
            const auto size = sizeof(T) * count;
            if (m_stage_buffers.empty()) {
                const auto required_size = math::ceil_power_of_two(size);

                gpu_stage_buffer stage;
                stage.init(m_gdevice, std::max<uint64>(required_size, 1_mib));
                m_stage_buffers.push_back(std::move(stage));
            }

            if (m_stage_buffers.back().remaining() >= size) {
                return m_stage_buffers.back().slice<T>(count);
            }

            const auto required_size = math::ceil_power_of_two(m_stage_buffers.back().capacity() + size);
            auto       stage = gpu_stage_buffer();
            stage.init(m_gdevice, std::max<uint64>(required_size, 1_mib));
            m_stage_buffers.push_back(std::move(stage));
            return m_stage_buffers.back().slice<T>(count);
        }

        void begin_frame(rhi::command_queue* upload_command_queue) noexcept;
        void end_frame() noexcept;

        rhi::command_queue* command_queue() noexcept;

    private:
        rhi::graphics_device*          m_gdevice = nullptr;
        rhi::command_queue*            m_upload_cmd_queue = nullptr;
        core::vector<gpu_stage_buffer> m_stage_buffers;
        uint64                         m_peak_usage = 0;
        uint32                         m_frames_since_resize = 0;
    };

} // namespace tavros::renderer