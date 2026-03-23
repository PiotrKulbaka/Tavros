#pragma once

#include <tavros/core/io/stream_base.hpp>

namespace tavros::core
{

    /**
     * @brief Base class for binary blob readers.
     *
     * Provides typed read methods over a binary stream.
     * Derived classes implement the low-level @c read(uint8*, size_t) method.
     *
     * State transitions:
     * - @c good -> @c eos  when read returns fewer bytes than requested
     * - @c good -> @c bad  when a zstring has no null terminator
     * - Both transitions are irreversible - create a new reader to retry.
     */
    class basic_stream_reader : public virtual stream_base
    {
    public:
        ~basic_stream_reader() override = default;

        /** @brief Returns true if the end of stream has been reached. */
        [[nodiscard]] bool eos() const noexcept
        {
            return m_state == stream_state::eos;
        }

        /**
         * @brief Reads raw bytes into @p dst.
         *
         * Returns the number of bytes actually read.
         * Does not modify reader state directly - derived classes must call @c set_state() when:
         * - Fewer bytes than requested were read due to end of stream -> @c stream_state::eos
         * - An unrecoverable I/O error occurred -> @c stream_state::bad
         * @throws file_error on I/O failure.
         */
        virtual size_t read(uint8* dst, size_t size) = 0;

        /**
         * @brief Reads into an existing variable @p out.
         *
         * @tparam T Must satisfy @c stream_readable (trivially copyable, not a string type).
         * Sets state to @c eos if the stream has insufficient data.
         * No-op if state is not @c good.
         */
        template<stream_readable T>
        void read(T& out)
        {
            if (!good()) {
                return;
            }

            auto bytes_read = read(reinterpret_cast<uint8*>(&out), sizeof(T));
            if (bytes_read != sizeof(T)) {
                set_state(stream_state::eos);
                out = {};
            }
        }

        /**
         * @brief Reads and returns a value of type @p T.
         *
         * @tparam T Must satisfy @c stream_readable (trivially copyable, not a string type).
         * Sets state to @c eos if the stream has insufficient data.
         * No-op and returns @c T{} if state is not @c good.
         */
        template<stream_readable T>
        [[nodiscard]] T read_as()
        {
            if (!good()) {
                return {};
            }

            T    out{};
            auto bytes_read = read(reinterpret_cast<uint8*>(&out), sizeof(T));
            if (bytes_read != sizeof(T)) {
                set_state(stream_state::eos);
                return {};
            }

            return out;
        }

        /**
         * @brief Reads a null-terminated string into a @c string.
         *
         * Reads bytes until @c \\0 is found.
         * If end of stream is reached before @c \\0 - partial content is returned, state -> @c bad.
         * No-op and returns @c {} if state is not @c good.
         */
        [[nodiscard]] string read_as_zstr()
        {
            if (!good()) {
                return {};
            }

            using char_t = string::value_type;
            string out;
            char_t ch{};

            while (true) {
                auto bytes_read = read(reinterpret_cast<uint8*>(&ch), sizeof(char_t));
                if (bytes_read != sizeof(char_t)) {
                    TAV_ASSERT(!good());
                    // read already set eos/bad, override to bad - missing \0 means corrupt data
                    set_state(stream_state::bad);
                    return out;
                }

                if (ch == static_cast<char_t>('\0')) {
                    return out;
                }
                out += ch;
            }
        }

        /**
         * @brief Reads a null-terminated string into a @c fixed_string<N>.
         *
         * Reads bytes until @c \\0 or @c N-1 characters are read.
         * If end of stream is reached before @c \\0 - partial content is returned, state -> @c bad.
         * If @c N-1 characters are read without @c \\0 - partial content is returned, state -> @c bad.
         * No-op and returns @c {} if state is not @c good.
         */
        template<size_t N>
        [[nodiscard]] fixed_string<N> read_as_zstr()
        {
            if (!good()) {
                return {};
            }

            using char_t = typename fixed_string<N>::value_type;
            fixed_string<N> out;
            char_t          ch{};

            while (true) {
                auto bytes_read = read(reinterpret_cast<uint8*>(&ch), sizeof(char_t));
                if (bytes_read != sizeof(char_t)) {
                    TAV_ASSERT(!good());
                    // read already set eos/bad, override to bad - missing \0 means corrupt data
                    set_state(stream_state::bad);
                    return out;
                }

                if (ch == static_cast<char_t>('\0')) {
                    return out;
                }

                out += ch;

                if (out.size() == N - 1) {
                    // Buffer full - peek next byte, it must be \0
                    auto bytes_read = read(reinterpret_cast<uint8*>(&ch), sizeof(char_t));
                    if (bytes_read != sizeof(char_t) || ch != static_cast<char_t>('\0')) {
                        set_state(stream_state::bad);
                    }
                    return out;
                }
            }
        }
    };

} // namespace tavros::core
