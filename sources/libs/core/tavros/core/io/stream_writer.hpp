#pragma once

#include <tavros/core/io/stream_base.hpp>

namespace tavros::core
{
    /**
     * @brief Base class for binary blob writers.
     *
     * Provides typed write methods over a binary stream.
     * Derived classes implement the low-level @c write(const uint8*, size_t) method.
     *
     * State transitions:
     * - @c good -> @c bad  when write fails or stream is full
     * - Transition is irreversible - create a new writer to retry.
     */
    class basic_stream_writer : public virtual stream_base
    {
    public:
        basic_stream_writer() noexcept = default;

        ~basic_stream_writer() override = default;

        /**
         * @brief Writes raw bytes from @p src.
         *
         * Returns the number of bytes actually written.
         * Derived classes must call @c set_state() when:
         * - Fewer bytes than requested were written -> @c stream_state::bad
         * - An unrecoverable I/O error occurred    -> @c stream_state::bad
         * @throws io_exception on I/O failure.
         */
        virtual size_t write(const uint8* src, size_t size) = 0;

        /**
         * @brief Writes a value from an existing variable.
         *
         * @tparam T Must satisfy @c stream_writable (trivially copyable, not a string type).
         * No-op if state is not @c good.
         */
        template<stream_writable T>
        void write(const T& val)
        {
            write_as<T>(val);
        }

        /**
         * @brief Writes a value of type @p T.
         *
         * @tparam T Must satisfy @c stream_writable (trivially copyable, not a string type).
         * No-op if state is not @c good.
         */
        template<stream_writable T>
        void write_as(const T& val)
        {
            if (!good()) {
                return;
            }
            auto written = write(reinterpret_cast<const uint8*>(&val), sizeof(T));
            if (written != sizeof(T)) {
                set_state(stream_state::bad);
            }
        }

        /**
         * @brief Writes a value of type @p T, casting @p val from type @p U.
         *
         * Useful for writing a value as a different type, e.g. @c write_as<float>(double_val).
         * @tparam T Target type to write as. Must satisfy @c stream_writable.
         * @tparam U Source type. Must be convertible to @p T.
         * No-op if state is not @c good.
         */
        template<stream_writable T, typename U>
        void write_as(const U& val)
        {
            write_as<T>(static_cast<T>(val));
        }

        /**
         * @brief Writes a @c string_view as a null-terminated string (including @c \\0).
         * No-op if state is not @c good.
         */
        void write_as_zstr(string_view val)
        {
            if (!good()) {
                return;
            }

            using char_t = typename string_view::value_type;

            auto written = write(reinterpret_cast<const uint8*>(val.data()), val.size() * sizeof(char_t));
            if (written != val.size()) {
                set_state(stream_state::bad);
                return;
            }

            auto null_ch = static_cast<char_t>('\0');
            if (write(reinterpret_cast<uint8*>(&null_ch), sizeof(char_t)) != sizeof(char_t)) {
                set_state(stream_state::bad);
            }
        }

    protected:
        /** @brief Sets the stream state. Used by derived classes and write methods. */
        void set_state(stream_state new_state) noexcept
        {
            m_state = new_state;
        }

    private:
        stream_state m_state = stream_state::good;
    };

} // namespace tavros::core
