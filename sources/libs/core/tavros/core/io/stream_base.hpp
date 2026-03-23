#pragma once

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/io/basic_io.hpp>

namespace tavros::core
{

    class stream_base
    {
    public:
        stream_base() noexcept = default;

        virtual ~stream_base() noexcept = default;

        /** @brief Returns true if the reader is in a valid state and can be read from. */
        [[nodiscard]] bool good() const noexcept
        {
            return m_state == stream_state::good;
        }

        /** @brief Returns true if the stream is in a bad state (corrupted data). */
        [[nodiscard]] bool bad() const noexcept
        {
            return m_state == stream_state::bad;
        }

        /** @brief Returns true if the reader is in a good state.Equivalent to @c good(). */
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return good();
        }

        /** @brief Returns true if the backend supports seeking. */
        [[nodiscard]] virtual bool seekable() const noexcept
        {
            return false;
        }

        /** @brief Seeks to a position in the stream. No-op if not seekable. */
        virtual bool seek(ssize_t offset, seek_dir dir = seek_dir::begin) noexcept
        {
            TAV_UNUSED(offset);
            TAV_UNUSED(dir);
            return false;
        }

        /** @brief Returns the current position, or -1 if not seekable. */
        [[nodiscard]] virtual ssize_t tell() const noexcept
        {
            return -1;
        }

        /** @brief Returns the total size of the stream, or 0 if not seekable. */
        [[nodiscard]] virtual size_t size() const noexcept
        {
            return 0;
        }

    protected:
        /** @brief Sets the stream state.Used by derived classes and read methods. */
        void set_state(stream_state new_state) noexcept
        {
            m_state = new_state;
        }

    protected:
        stream_state m_state = stream_state::good;
    };

} // namespace tavros::core
