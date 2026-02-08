#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/renderer/gpu_buffer_view.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

namespace tavros::renderer
{

    /**
     * @brief Persistent CPU-to-GPU streaming buffer.
     *
     * gpu_stream_buffer provides a linear allocator over a persistently mapped GPU buffer.
     * It is intended for frequently updated per-frame data such as:
     * - Uniform buffers
     * - Dynamic vertex or index data
     *
     * Memory is allocated sequentially and reset every frame via begin_frame().
     *
     * @note This class is non-copyable.
     * @note Not thread-safe.
     */
    class gpu_stream_buffer : tavros::core::noncopyable
    {
    public:
        /**
         * @brief Creates a streaming GPU buffer and maps it persistently.
         *
         * @param gdevice Pointer to the graphics device.
         * @param size    Total buffer size in bytes.
         * @param usage   Intended GPU usage flags.
         */
        explicit gpu_stream_buffer(core::raw_ptr<rhi::graphics_device> gdevice, size_t size, rhi::buffer_usage usage)
            : m_gdevice(gdevice)
            , m_gpu_buffer()
            , m_data()
            , m_cursor(0)
        {
            TAV_ASSERT(gdevice);
            rhi::buffer_create_info buffer_desc{size, usage, rhi::buffer_access::cpu_to_gpu};
            m_gpu_buffer = m_gdevice->create_buffer(buffer_desc);
            m_data = m_gdevice->map_buffer(m_gpu_buffer);
        }

        /**
         * @brief Move constructor.
         */
        gpu_stream_buffer(gpu_stream_buffer&&) noexcept = default;

        /**
         * @brief Unmaps and destroys the underlying GPU buffer.
         */
        ~gpu_stream_buffer() noexcept
        {
            if (m_gdevice) {
                m_gdevice->unmap_buffer(m_gpu_buffer);
                m_gdevice->safe_destroy(m_gpu_buffer);
            }
        }

        /**
         * @brief Move assignment.
         */
        gpu_stream_buffer& operator=(gpu_stream_buffer&& other) noexcept = default;

        /**
         * @brief Resets the internal allocation cursor.
         *
         * Typically called once per frame before issuing new allocations.
         */
        void begin_frame() noexcept
        {
            m_cursor = 0;
        }

        /**
         * @brief Returns the total buffer capacity in bytes.
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
            return m_data.size();
        }

        /**
         * @brief Returns the remaining available space in bytes.
         */
        [[nodiscard]] size_t remaining() const noexcept
        {
            return m_data.size() - m_cursor;
        }

        /**
         * @brief Returns the number of bytes already allocated.
         */
        [[nodiscard]] size_t used() const noexcept
        {
            return m_cursor;
        }

        /**
         * @brief Allocates a typed slice from the stream buffer.
         *
         * The returned slice is aligned to alignof(T).
         *
         * @tparam T     Element type.
         * @param count  Number of elements to allocate.
         *
         * @return gpu_buffer_view<T> describing the allocated subrange.
         *         Returns an empty view if there is insufficient space.
         *
         * @note Allocation is linear and cannot be freed individually.
         */
        template<class T>
        [[nodiscard]] gpu_buffer_view<T> slice(size_t count) noexcept
        {
            size_t offset = tavros::math::align_up(m_cursor, alignof(T));
            size_t size_bytes = sizeof(T) * count;

            TAV_ASSERT(offset + size_bytes <= m_data.size());
            TAV_ASSERT(reinterpret_cast<uintptr_t>(m_data.begin() + offset) % alignof(T) == 0);
            if (offset + size_bytes > m_data.size()) {
                return gpu_buffer_view<T>{};
            }

            m_cursor = offset + size_bytes;
            T* begin = reinterpret_cast<T*>(m_data.begin() + offset);
            return gpu_buffer_view<T>{m_gpu_buffer, offset, tavros::core::buffer_span<T>{begin, count}};
        }

    private:
        core::raw_ptr<rhi::graphics_device> m_gdevice;
        rhi::buffer_handle                  m_gpu_buffer;
        tavros::core::buffer_span<uint8>    m_data; // persistent data
        size_t                              m_cursor;
    };

} // namespace tavros::renderer
