#include <tavros/renderer/upload_context.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("upload_context");
}

namespace tavros::renderer
{

    upload_context::upload_context(rhi::graphics_device* gdevice) noexcept
        : m_gdevice(gdevice)
    {
    }

    upload_context::~upload_context() noexcept
    {
        for (auto& stage : m_stage_buffers) {
            stage.shutdown();
        }
        m_stage_buffers.clear();
    }

    void upload_context::begin_frame(rhi::command_queue* upload_command_queue) noexcept
    {
        if (m_upload_cmd_queue) {
            logger.warning("previous frame is not ended");
        }
        m_upload_cmd_queue = upload_command_queue;

        // Consolidate multiple stage buffers.
        if (m_stage_buffers.size() > 1) {
            uint64 required_size = 0;

            for (auto& stage : m_stage_buffers) {
                required_size += stage.size();
            }

            required_size = math::ceil_power_of_two(required_size);

            gpu_stage_buffer new_stage;
            new_stage.init(m_gdevice, required_size);

            for (auto& stage : m_stage_buffers) {
                stage.shutdown();
            }

            m_stage_buffers.clear();
            m_stage_buffers.push_back(std::move(new_stage));

            m_peak_usage = 0;
            m_frames_since_resize = 0;
        }

        // Reset usage for current frame.
        for (auto& stage : m_stage_buffers) {
            stage.reset();
        }
    }

    void upload_context::end_frame() noexcept
    {
        uint64 total_used = 0;
        uint64 total_capacity = 0;

        for (auto& stage : m_stage_buffers) {
            total_used += stage.size();
            total_capacity += stage.capacity();
        }

        m_peak_usage = std::max(m_peak_usage, total_used);

        ++m_frames_since_resize;

        // Shrink oversized staging buffer.
        if (m_stage_buffers.size() == 1 && m_frames_since_resize >= 100 && total_capacity > 0) {
            const double usage_ratio = static_cast<double>(m_peak_usage) / static_cast<double>(total_capacity);

            if (usage_ratio == 0.0) {
                TAV_ASSERT(m_stage_buffers.size() == 1);
                m_stage_buffers.back().shutdown();
                m_stage_buffers.clear();
            } else if (usage_ratio <= 0.45) {
                uint64 new_size = math::ceil_power_of_two(m_peak_usage);
                new_size = std::max<uint64>(new_size, 1_mib);
                if (new_size < total_capacity) {
                    gpu_stage_buffer new_stage;
                    new_stage.init(m_gdevice, new_size);

                    m_stage_buffers.back().shutdown();
                    m_stage_buffers.clear();
                    m_stage_buffers.push_back(std::move(new_stage));
                }
            }

            m_peak_usage = 0;
            m_frames_since_resize = 0;
        }

        m_upload_cmd_queue = nullptr;
    }

    rhi::command_queue* upload_context::command_queue() noexcept
    {
        return m_upload_cmd_queue;
    }

} // namespace tavros::renderer
