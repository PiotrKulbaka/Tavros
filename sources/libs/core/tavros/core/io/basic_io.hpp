#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/fixed_string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/traits.hpp>

namespace tavros::core
{

    /**
     * @brief Represents the reference point for seeking within a stream.
     */
    enum class seek_dir
    {
        /// Seek relative to the beginning of the stream.
        begin,

        /// Seek relative to the current position in the stream.
        current,

        /// Seek relative to the end of the stream.
        end,
    };

    /**
     * @brief Stream state flags describing the result of read operations.
     *
     * Used to indicate whether a read succeeded, reached end of stream,
     * or encountered invalid/corrupted data.
     */
    enum class stream_state : uint8
    {
        /// @brief Read operation completed successfully.
        good = 0,

        /// @brief End of stream reached before requested amount was read.
        eos,

        /// @brief Invalid or corrupted data encountered.
        bad,
    };

    /**
     * @brief Concept for types readable as raw binary blobs.
     *
     * Satisfied by trivially copyable, non-string types.
     * Intended for direct memory reads without parsing or validation.
     */
    template<typename T>
    concept stream_readable = std::is_trivially_copyable_v<T> && !is_string_type_v<T>;

    /**
     * @brief Concept for types writable as raw binary blobs.
     *
     * Satisfied by trivially copyable, non-string types.
     * Intended for direct memory writes without parsing or validation.
     */
    template<typename T>
    concept stream_writable = std::is_trivially_copyable_v<T> && !is_string_type_v<T>;

    /** Returns a string representation of the stream state. */
    string_view to_string(stream_state state) noexcept;

    /** Returns a string representation of the seek direction. */
    string_view to_string(seek_dir dir) noexcept;

} // namespace tavros::core
