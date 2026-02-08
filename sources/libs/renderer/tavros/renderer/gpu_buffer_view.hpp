#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/memory/buffer_span.hpp>
#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer
{

    /**
     * @brief Lightweight view over a subrange of a GPU buffer.
     *
     * gpu_buffer_view represents a typed slice of a GPU buffer with a fixed byte offset
     * and element count. It does not own the buffer or the memory it refers to.
     *
     * Typical usage:
     * - Returned by gpu_stream_buffer::slice()
     * - Used to bind subranges of buffers to the GPU pipeline
     *
     * @tparam T Element type of the buffer view.
     *
     * @note The lifetime of this view is bound to the lifetime of the underlying GPU buffer
     *       and its mapped memory.
     */
    template<class T>
    class gpu_buffer_view
    {
    public:
        /**
         * @brief Constructs an empty (invalid) buffer view.
         */
        gpu_buffer_view() noexcept = default;

        /**
         * @brief Constructs a buffer view over a specific buffer subrange.
         *
         * @param buffer       GPU buffer handle.
         * @param offset_bytes Byte offset from the start of the buffer.
         * @param slice        CPU-visible span representing the mapped data.
         */
        explicit gpu_buffer_view(rhi::buffer_handle buffer, size_t offset_bytes, core::buffer_span<T> slice) noexcept
            : m_buffer(buffer)
            , m_offset(offset_bytes)
            , m_slice(slice)
        {
        }

        /** Destructor. */
        ~gpu_buffer_view() noexcept = default;

        /**
         * @brief Returns the underlying GPU buffer handle.
         */
        [[nodiscard]] rhi::buffer_handle gpu_buffer() const noexcept
        {
            return m_buffer;
        }

        /**
         * @brief Returns the byte offset of this view within the GPU buffer.
         */
        [[nodiscard]] size_t offset_bytes() const noexcept
        {
            return m_offset;
        }

        /**
         * @brief Returns the size of the view in bytes.
         */
        [[nodiscard]] size_t size_bytes() const noexcept
        {
            return sizeof(T) * m_slice.size();
        }

        /**
         * @brief Returns a CPU-visible span to the mapped buffer data.
         *
         * @note This span is valid only while the underlying buffer remains mapped.
         */
        [[nodiscard]] core::buffer_span<T> data() const noexcept
        {
            return m_slice;
        }

    private:
        rhi::buffer_handle   m_buffer;
        size_t               m_offset = 0;
        core::buffer_span<T> m_slice;
    };

} // namespace tavros::renderer
