#pragma once

#include <tavros/core/defines.hpp>

namespace tavros::debug
{

/**
 * @brief A wrapper type that exists only in debug builds.
 * @ingroup debug
 *
 * @tparam T The type of value to wrap.
 *
 * This template is designed to hold a value that is only available in debug
 * builds. In release builds, the wrapper becomes a zero-sized type that discards
 * any value passed to it, eliminating runtime overhead.
 *
 * ### Example usage:
 * @code
 * debug_only<int> frame_counter = 0;
 * debug_only<std::string> debug_name = "MainPass";
 * @endcode
 */
#if TAV_DEBUG
    template<typename T>
    class debug_only
    {
    public:
        constexpr debug_only() noexcept = default;

        /** @param value The value to store. */
        constexpr debug_only(T value) noexcept
            : m_value(value)
        {
        }

        /** @brief Accesses the wrapped value. */
        constexpr T& operator*() noexcept
        {
            return m_value;
        }

        /** @brief Accesses the wrapped value (const version). */
        constexpr const T& operator*() const noexcept
        {
            return m_value;
        }

        /** @brief Accesses the wrapped value via pointer. */
        constexpr T* operator->() noexcept
        {
            return &m_value;
        }

        /** @brief Implicit conversion to the wrapped type. */
        constexpr operator T&() noexcept
        {
            return m_value;
        }

        /** @brief Implicit conversion to the wrapped type (const version). */
        constexpr operator const T&() const noexcept
        {
            return m_value;
        }

    private:
        T m_value{};
    };
#else
    template<typename T>
    class debug_only
    {
    public:
        constexpr debug_only() noexcept = default;
        constexpr debug_only(T) noexcept
        {
        }
    };
#endif

/**
 * @brief Macro for conditionally including code in debug builds only.
 * @ingroup debug
 *
 * This macro evaluates to its argument in debug builds and expands to nothing
 * in release builds. It provides a convenient way to conditionally compile
 * debug-specific statements.
 *
 * ### Example usage:
 * @code
 * TAV_DEBUG_ONLY(glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "RenderPass"));
 *
 * // Multiple statements
 * TAV_DEBUG_ONLY(
 *     glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Opaque");
 *     render_opaque();
 *     glPopDebugGroup();
 * )
 * @endcode
 *
 * @param ... The code to conditionally include in debug builds.
 */
#if TAV_DEBUG
    #define TAV_DEBUG_ONLY(...) __VA_ARGS__;
#else
    #define TAV_DEBUG_ONLY(...)
#endif

} // namespace tavros::debug