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
        // Ensure GPU is done before releasing staging buffers
        flush();
        m_gdevice->safe_destroy(m_fence);
    }

    void upload_context::flush() noexcept
    {
        if (m_current_upload_queue) {
            if (!m_fence) {
                m_fence = m_gdevice->create_fence();
            }
            m_current_upload_queue->signal_fence(m_fence);
            m_gdevice->submit_command_queue(m_current_upload_queue);
            m_gdevice->client_wait_for_fence(m_fence);
            m_stage_buffer.reset();
            if (m_large_stage_buffer.capacity()) {
                m_large_stage_buffer.shutdown();
            }
            m_current_upload_queue = nullptr;
        }
    }

    upload_context::upload_batch upload_context::slice(size_t size) noexcept
    {
        if (!m_stage_buffer.capacity()) {
            m_stage_buffer.init(m_gdevice, k_stage_buffer_size);
            TAV_ASSERT(m_stage_buffer.capacity() == k_stage_buffer_size);
        }

        if (m_stage_buffer.remaining() >= size) {
            if (!m_current_upload_queue) {
                m_current_upload_queue = m_gdevice->create_command_queue();
                TAV_ASSERT(m_current_upload_queue);
            }
            return upload_batch{m_stage_buffer.slice<uint8>(size), m_current_upload_queue};
        }

        flush();

        if (!m_current_upload_queue) {
            m_current_upload_queue = m_gdevice->create_command_queue();
            TAV_ASSERT(m_current_upload_queue);
        }

        // Try one more time
        if (m_stage_buffer.remaining() >= size) {
            return upload_batch{m_stage_buffer.slice<uint8>(size), m_current_upload_queue};
        }

        // Last attempt, allocate a separate buffer for this
        m_large_stage_buffer.init(m_gdevice, size);
        return upload_batch{m_large_stage_buffer.slice<uint8>(size), m_current_upload_queue};
    }

} // namespace tavros::renderer
