#pragma once

#include <cstddef>
#include <atomic>

using uint8 = uint8_t;
using int8 = int8_t;
using uint16 = uint16_t;
using int16 = int16_t;
using uint32 = uint32_t;
using int32 = int32_t;
using uint64 = uint64_t;
using int64 = int64_t;
using size_t = std::size_t;
using ssize_t = int64_t;
using ptrdiff_t = std::ptrdiff_t;
using char32 = char32_t;

using atomic_uint8 = std::atomic_uint8_t;
using atomic_int8 = std::atomic_int8_t;
using atomic_uint16 = std::atomic_uint16_t;
using atomic_int16 = std::atomic_int16_t;
using atomic_uint32 = std::atomic_uint32_t;
using atomic_int32 = std::atomic_int32_t;
using atomic_uint64 = std::atomic_uint64_t;
using atomic_int64 = std::atomic_int64_t;
using atomic_size_t = std::atomic_size_t;
using atomic_bool = std::atomic_bool;

constexpr size_t pointer_size = sizeof(void*);

static_assert(sizeof(uint8) == 1);
static_assert(sizeof(int8) == 1);
static_assert(sizeof(uint16) == 2);
static_assert(sizeof(int16) == 2);
static_assert(sizeof(uint32) == 4);
static_assert(sizeof(int32) == 4);
static_assert(sizeof(uint64) == 8);
static_assert(sizeof(int64) == 8);
static_assert(sizeof(size_t) == sizeof(void*));

#define TAV_UNUSED(x) ((void) (x))

namespace tavros::core
{

    /**
     * @brief Tag type used to explicitly enable unsafe operations.
     *
     * This tag is passed to functions that perform non-standard or potentially
     * unsafe operations, typically for performance or low-level memory access.
     *
     * By providing this tag, the caller explicitly acknowledges that:
     * - required invariants are satisfied,
     * - no additional safety checks are performed,
     * - undefined behavior may occur if used incorrectly.
     *
     * This mechanism is intended to make unsafe code paths explicit at call sites,
     * preventing accidental misuse while still allowing advanced optimizations.
     *
     * Example:
     * @code
     * buffer.write(ptr, size, unsafe); // caller guarantees validity
     * @endcode
     */
    struct unsafe_t
    {
        explicit unsafe_t() = default;
    };

    /**
     * @brief Global constant instance of @ref unsafe_t.
     *
     * Use this object to opt into unsafe operations.
     */
    inline constexpr unsafe_t unsafe{};

    /**
     * @brief Tag type that enables truncation on overflow.
     *
     * This tag is used to indicate that, in case of insufficient capacity,
     * the operation should not fail or assert, but instead truncate the input
     * to fit into the available space.
     *
     * When this tag is provided, the caller explicitly allows loss of data
     * beyond the container's capacity.
     *
     * This is useful for scenarios where:
     * - fixed-size buffers are used,
     * - partial writes are acceptable,
     * - performance is preferred over strict correctness.
     *
     * Example:
     * @code
     * static_string<8>::format("Hello World!!!"); // excess data will be discarded
     * @endcode
     */
    struct on_overflow_truncate_t
    {
        explicit on_overflow_truncate_t() = default;
    };

    /**
     * @brief Global constant instance of @ref on_overflow_truncate_t.
     *
     * Use this object to enable truncation behavior on overflow.
     */
    inline constexpr on_overflow_truncate_t on_overflow_truncate{};
} // namespace tavros::core
