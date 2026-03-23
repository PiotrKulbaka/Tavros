#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/renderer/gpu_buffer_view.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

namespace tavros::renderer
{

    /**
     * @brief Base class for GPU persistent buffers with linear slice allocation.
     *
     * Owns a single GPU buffer and a persistent CPU mapping.
     * Provides typed slice allocation via slice<T>().
     *
     * Not intended to be used directly - use derived classes:
     * - gpu_stream_buffer  : per-frame streaming (vertex, uniform)
     * - gpu_stage_buffer : CPU-to-GPU upload (textures, static meshes)
     * - gpu_readback_buffer: GPU-to-CPU readback (screenshots, compute results)
     */
    class basic_gpu_persistent_buffer : core::noncopyable
    {
    public:
        /** @brief Default constructor. */
        explicit basic_gpu_persistent_buffer() noexcept = default;

        /** @brief Move constructor. */
        basic_gpu_persistent_buffer(basic_gpu_persistent_buffer&& other) noexcept
            : m_gdevice(other.m_gdevice)
            , m_buffer(other.m_buffer)
            , m_data(other.m_data)
            , m_cursor(other.m_cursor)
        {
            other.m_gdevice = nullptr;
            other.m_buffer = {};
            other.m_data = {};
            other.m_cursor = 0;
        }

        /**
         * @brief Unmaps and destroys the underlying GPU buffer.
         */
        ~basic_gpu_persistent_buffer() noexcept
        {
            destroy_all();
        }

        /**
         * @brief Move assignment.
         */
        basic_gpu_persistent_buffer& operator=(basic_gpu_persistent_buffer&& other) noexcept
        {
            if (this == &other) {
                return *this;
            }

            destroy_all();

            m_gdevice = other.m_gdevice;
            m_buffer = other.m_buffer;
            m_data = other.m_data;
            m_cursor = other.m_cursor;

            other.m_gdevice = nullptr;
            other.m_buffer = {};
            other.m_data = {};
            other.m_cursor = 0;
        }

        /**
         * @brief Allocates a typed slice from the buffer.
         *
         * Allocation is linear. Use reset() to reclaim all memory.
         * The returned slice is aligned to alignof(T).
         *
         * @tparam T     Element type.
         * @param count  Number of elements to allocate.
         * @return       gpu_buffer_view<T> over the allocated subrange.
         */
        template<class T>
        [[nodiscard]] gpu_buffer_view<T> slice(size_t count) noexcept
        {
            const size_t offset = tavros::math::align_up(m_cursor, alignof(T));
            const size_t size_bytes = sizeof(T) * count;

            TAV_ASSERT(offset + size_bytes <= m_data.size());
            if (offset + size_bytes > m_data.size()) {
                return gpu_buffer_view<T>{};
            }

            m_cursor = offset + size_bytes;
            T* begin = reinterpret_cast<T*>(m_data.begin() + offset);
            return gpu_buffer_view<T>{m_buffer, offset, core::buffer_span<T>{begin, count}};
        }

        /**
         * @brief Returns the total buffer capacity in bytes.
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
            return m_data.size();
        }

        /**
         * @brief Returns the number of bytes already allocated.
         */
        [[nodiscard]] size_t size() const noexcept
        {
            return m_cursor;
        }

        /**
         * @brief Returns the remaining available space in bytes.
         */
        [[nodiscard]] size_t remaining() const noexcept
        {
            return m_data.size() - m_cursor;
        }

        /**
         * @brief Returns the underlying GPU buffer handle.
         */
        [[nodiscard]] rhi::buffer_handle gpu_buffer() const noexcept
        {
            return m_buffer;
        }

        /**
         * @brief Resets the internal allocation cursor.
         *
         * Reclaims all previously allocated slices, making the entire buffer
         * available for new allocations.
         *
         * @pre The GPU must have finished consuming all data uploaded in previous slices.
         *      Call only after waiting on the fence associated with the last submit.
         */
        void reset() noexcept
        {
            m_cursor = 0;
        }

    protected:
        void internal_init(rhi::graphics_device* gdevice, size_t size, rhi::buffer_usage usage, rhi::buffer_access access) noexcept
        {
            TAV_ASSERT(gdevice);
            TAV_ASSERT(m_gdevice == nullptr);
            m_gdevice = gdevice;
            rhi::buffer_create_info info{size, usage, access};
            m_buffer = m_gdevice->create_buffer(info);
            m_data = m_gdevice->map_buffer(m_buffer);
        }

        void destroy_all() noexcept
        {
            if (m_gdevice) {
                m_gdevice->unmap_buffer(m_buffer);
                m_gdevice->safe_destroy(m_buffer);
                m_gdevice = nullptr;
            }
            m_buffer = {};
            m_data = {};
            m_cursor = 0;
        }

    private:
        rhi::graphics_device*    m_gdevice = nullptr;
        rhi::buffer_handle       m_buffer;
        core::buffer_span<uint8> m_data;
        size_t                   m_cursor = 0;
    };

} // namespace tavros::renderer
