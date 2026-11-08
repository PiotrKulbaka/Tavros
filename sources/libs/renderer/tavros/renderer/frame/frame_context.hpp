#pragma once

#include <tavros/renderer/frame/frame_data.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/renderer/resource_manager.hpp>

namespace tavros::renderer
{

    class render_system;

    class frame_context
    {
    public:
        ~frame_context() noexcept = default;

        frame_context(frame_context&& other) noexcept
            : m_gd(other.m_gd)
            , m_rm(other.m_rm)
            , m_cq(other.m_cq)
            , m_sb(other.m_sb)
            , m_fn(other.m_fn)
            , m_fd(std::move(other.m_fd))
        {
            other.m_gd = nullptr;
            other.m_rm = nullptr;
            other.m_cq = nullptr;
            other.m_sb = nullptr;
            other.m_fn = 0;
        }

        frame_context& operator=(frame_context&& other) noexcept
        {
            if (this == &other) {
                return *this;
            }

            m_gd = other.m_gd;
            m_rm = other.m_rm;
            m_cq = other.m_cq;
            m_sb = other.m_sb;
            m_fn = other.m_fn;
            m_fd = std::move(other.m_fd);

            other.m_gd = nullptr;
            other.m_rm = nullptr;
            other.m_cq = nullptr;
            other.m_sb = nullptr;
            other.m_fn = 0;

            return *this;
        }

        rhi::graphics_device& graphics_device() noexcept
        {
            return *m_gd;
        }

        rhi::command_queue& commands() noexcept
        {
            return *m_cq;
        }

        gpu_stream_buffer& stream_buffer() noexcept
        {
            return *m_sb;
        }

        renderer::resource_manager& resource_manager() noexcept
        {
            return *m_rm;
        }

        uint64 frame_number() const noexcept
        {
            return m_fn;
        }

        frame_data_t& frame_data() noexcept
        {
            return m_fd;
        }

    private:
        friend class render_system;

        frame_context() noexcept = default;

        frame_context(rhi::graphics_device* gd, renderer::resource_manager* rm, gpu_stream_buffer* sb, uint64 frame_number) noexcept
            : m_gd(gd)
            , m_rm(rm)
            , m_sb(sb)
            , m_fn(frame_number)
        {
            m_cq = m_gd->create_command_queue();
            TAV_ASSERT(m_rm);
            TAV_ASSERT(m_cq);
            TAV_ASSERT(m_sb);
        }

        void clean() noexcept
        {
            m_gd = nullptr;
            m_rm = nullptr;
            m_cq = nullptr;
            m_sb = nullptr;
            m_fn = 0;
            m_fd.clear();
        }

    private:
        rhi::graphics_device*       m_gd = nullptr;
        renderer::resource_manager* m_rm = nullptr;
        rhi::command_queue*         m_cq = nullptr;
        gpu_stream_buffer*          m_sb = nullptr;
        uint64                      m_fn = 0;
        frame_data_t                m_fd;
    };

} // namespace tavros::renderer
