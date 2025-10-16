#pragma once

#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/verify.hpp>

#include <type_traits>
#include <initializer_list>
#include <iterator>
#include <array>

namespace tavros::core
{

    /**
     * @brief A fixed-capacity, stack-allocated vector container.
     *
     * This class provides an STL-like interface similar to `vector`,
     * but stores elements in a statically allocated buffer of size `N`.
     * No dynamic memory allocations are performed.
     *
     * @tparam T Element type. Must be nothrow move-constructible.
     * @tparam N Maximum number of elements that can be stored in the container.
     */
    template<typename T, size_t N>
        requires std::is_nothrow_move_constructible_v<T>
    class static_vector
    {
    public:
        using iterator = T*;                                                  /// Mutable iterator type.
        using const_iterator = const T*;                                      /// Immutable iterator type.
        using reverse_iterator = std::reverse_iterator<iterator>;             /// Mutable reverse iterator.
        using const_reverse_iterator = std::reverse_iterator<const_iterator>; /// Immutable reverse iterator.

    public:
        /**
         * @brief Default constructor. Initializes an empty container.
         */
        constexpr static_vector() noexcept = default;

        /**
         * @brief Constructs the container from an initializer list.
         *
         * @param init Initializer list of elements to insert.
         * @note The number of elements must not exceed `N`.
         */
        constexpr explicit static_vector(std::initializer_list<T> init) noexcept
        {
            TAV_ASSERT(init.size() <= N);
            for (const auto& value : init) {
                emplace_back(value);
            }
        }

        /**
         * @brief Constructs the container from a buffer_view.
         *
         * @param init Span of elements to copy into the container.
         * @note The number of elements must not exceed `N`.
         */
        constexpr explicit static_vector(buffer_view<T> init) noexcept
        {
            TAV_ASSERT(init.size() <= N);
            for (const auto& value : init) {
                emplace_back(value);
            }
        }

        /**
         * @brief Swaps contents with another static_vector.
         *
         * @param other Another container to swap contents with.
         * @note Swapping is `noexcept` if `T` is nothrow swappable.
         */
        constexpr void swap(static_vector& other) noexcept(std::is_nothrow_swappable_v<T>)
        {
            m_data.swap(other.m_data);
            auto sz = m_size;
            m_size = other.m_size;
            other.m_size = sz;
        }

        constexpr iterator begin() noexcept
        {
            return m_data.data();
        }

        constexpr const_iterator begin() const noexcept
        {
            return m_data.data();
        }

        constexpr iterator end() noexcept
        {
            return m_data.data() + m_size;
        }

        constexpr const_iterator end() const noexcept
        {
            return m_data.data() + m_size;
        }

        constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        constexpr const_iterator cbegin() const noexcept
        {
            return begin();
        }

        constexpr const_iterator cend() const noexcept
        {
            return end();
        }

        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        constexpr const_reverse_iterator crend() const noexcept
        {
            return rend();
        }

        /**
         * @brief Returns the current number of elements.
         */
        constexpr size_t size() const noexcept
        {
            return m_size;
        }

        /**
         * @brief Returns the maximum number of elements the container can hold.
         */
        constexpr size_t max_size() const noexcept
        {
            return N;
        }

        /**
         * @brief Checks whether the container is empty.
         */
        constexpr bool empty() const noexcept
        {
            return m_size == 0;
        }

        constexpr T& at(size_t p) noexcept
        {
            TAV_VERIFY(p < m_size);
            return m_data.at(p);
        }

        constexpr const T& at(size_t p) const noexcept
        {
            TAV_VERIFY(p < m_size);
            return m_data.at(p);
        }

        constexpr T& operator[](size_t p) noexcept
        {
            TAV_ASSERT(p < m_size);
            return m_data[p];
        }

        constexpr const T& operator[](size_t p) const noexcept
        {
            TAV_ASSERT(p < m_size);
            return m_data[p];
        }

        constexpr T& front() noexcept
        {
            return *begin();
        }

        constexpr const T& front() const noexcept
        {
            return *begin();
        }

        constexpr T& back() noexcept
        {
            return *(end() - 1);
        }

        constexpr const T& back() const noexcept
        {
            return *(end() - 1);
        }

        /**
         * @brief Adds a copy of the given value to the end.
         *
         * @param value Element to add.
         * @note Behavior is undefined if size exceeds N.
         */
        constexpr void push_back(const T& value) noexcept
        {
            TAV_VERIFY(m_size < N);
            m_data[m_size++] = value;
        }

        /**
         * @brief Adds a moved value to the end.
         *
         * @param value Element to move.
         * @note Behavior is undefined if size exceeds N.
         */
        constexpr void push_back(T&& value) noexcept
        {
            TAV_VERIFY(m_size < N);
            m_data[m_size++] = std::move(value);
        }

        /**
         * @brief Removes the last element.
         *
         * @note Behavior is undefined if the container is empty.
         */
        constexpr void pop_back() noexcept
        {
            TAV_VERIFY(m_size > 0);
            --m_size;
            m_data[m_size] = T{};
        }

        /**
         * @brief Constructs a new element in place at the end.
         *
         * @tparam Args Argument types for T's constructor.
         * @param args Arguments to forward to T's constructor.
         * @return Reference to the newly constructed element.
         */
        template<typename... Args>
        constexpr T& emplace_back(Args&&... args) noexcept
        {
            TAV_VERIFY(m_size < N);
            new (&m_data[m_size]) T(std::forward<Args>(args)...);
            return m_data[m_size++];
        }

        /**
         * @brief Clears the container.
         *
         * @note Does not destroy elements explicitly; only resets the size.
         */
        constexpr void clear() noexcept
        {
            m_size = 0;
        }

        constexpr T* data() noexcept
        {
            return m_data.data();
        }

        constexpr const T* data() const noexcept
        {
            return m_data.data();
        }

    private:
        std::array<T, N> m_data{};
        size_t           m_size = 0;
    };

} // namespace tavros::core
