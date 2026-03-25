#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/debug/assert.hpp>

#include <concepts>

namespace tavros::core
{

    /**
     * @brief Thread-safe reference counter.
     *
     * Uses atomic operations for increment/decrement.
     */
    struct atomic_ref_count
    {
        void increment() noexcept
        {
            ++m_count;
        }

        bool decrement() noexcept
        {
            return --m_count == 0;
        }

        int32 load() const noexcept
        {
            return m_count.load();
        }

    private:
        std::atomic<int32> m_count{1};
    };

    /**
     * @brief Non-thread-safe reference counter.
     *
     * Intended for single-threaded usage or mutex protected.
     */
    struct single_thread_ref_count
    {
        void increment() noexcept
        {
            ++m_count;
        }

        bool decrement() noexcept
        {
            TAV_ASSERT(m_count > 0);
            return --m_count == 0;
        }

        int32 load() const noexcept
        {
            TAV_ASSERT(m_count >= 0);
            return m_count;
        }

    private:
        int32 m_count{1};
    };

    /**
     * @brief Reference-counted wrapper for a value.
     *
     * Provides intrusive-style reference counting via acquire/release.
     * RefCount policy defines thread-safety behavior.
     *
     * @tparam T        Stored value type.
     * @tparam RefCount Reference counter policy.
     */
    template<class T, class RefCount = single_thread_ref_count>
        requires requires(RefCount rc) {
            { rc.increment() } -> std::same_as<void>;
            { rc.decrement() } -> std::same_as<bool>;
            { rc.load() } -> std::convertible_to<int32>;
        } && std::is_nothrow_move_constructible_v<T>
    class ref_counted
    {
    public:
        /**
         * @brief Constructs the wrapper, taking ownership of the value.
         * @param value Value to store. Moved into the wrapper.
         */
        explicit ref_counted(T value) noexcept
            : m_value(std::move(value))
        {
        }

        /** @brief Increments the reference count. */
        void acquire() noexcept
        {
            m_count.increment();
        }

        /** @brief Decrements the reference count. Returns true if the count reached zero and the object may be deleted. */
        bool release() noexcept
        {
            TAV_ASSERT(m_count.load() > 0);
            return m_count.decrement();
        }

        /** @brief Returns the current reference count. */
        int32 count() const noexcept
        {
            return m_count.load();
        }

        /** @brief Returns a mutable reference to the stored value. */
        T& value() noexcept
        {
            return m_value;
        }

        /** @brief Returns a const reference to the stored value. */
        const T& value() const noexcept
        {
            return m_value;
        }

    private:
        T        m_value;
        RefCount m_count;
    };

} // namespace tavros::core
