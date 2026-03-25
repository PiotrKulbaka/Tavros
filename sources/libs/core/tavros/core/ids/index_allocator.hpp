#pragma once

#include <tavros/core/ids/index_base.hpp>
#include <tavros/core/noncopyable.hpp>

#include <concepts>
#include <iterator>

namespace tavros::core
{

    /**
     * @brief Concept that constrains a type to be a valid index allocator.
     *
     * A type satisfies this concept if it provides the full set of allocation,
     * query, and iteration operations expected by the index allocator contract.
     *
     * @tparam T  Type to check.
     */
    template<typename T>
    concept index_allocator_concept =
        requires(T a, const T ca, index_t idx) {
            { a.allocate() } -> std::same_as<index_t>;
            { a.deallocate(idx) } -> std::same_as<bool>;
            { ca.contains(idx) } -> std::same_as<bool>;
            { a.reset() };
            { ca.capacity() } -> std::same_as<size_t>;
            { ca.size() } -> std::same_as<size_t>;
            { ca.empty() } -> std::same_as<bool>;
            { ca.full() } -> std::same_as<bool>;
            { ca.available() } -> std::same_as<size_t>;
            { a.begin() };
            { ca.begin() };
            { a.end() };
            { ca.end() };
        };

    /**
     * @brief CRTP base class for index allocators.
     *
     * Provides a default implementation of common query methods (@ref empty,
     * @ref full, @ref available) and STL-compatible forward iterators, so that
     * concrete allocators only need to implement the core allocation primitives
     * and two navigation helpers:
     *
     * - `index_t find_first() const noexcept` - index of the first allocated slot,
     *   or `capacity()` if none are allocated.
     * - `index_t next_after(index_t pos) const noexcept` - index of the next
     *   allocated slot after @p pos, or `capacity()` if there are no more.
     *
     * @par Iterator invalidation
     * Iterators are invalidated by any call to @ref allocate, @ref deallocate,
     * or @ref reset on the owning allocator.
     *
     * @tparam Derived  Concrete allocator type. Must expose `size()`, `capacity()`,
     *                  `find_first()`, and `next_after()`.
     */
    template<class Derived>
    class index_allocator_base : noncopyable
    {
    public:
        /**
         * @brief STL-compatible forward iterator over allocated indices.
         *
         * Iterates only over currently allocated indices in ascending order.
         * Dereferencing yields the @ref index_t value of the allocated slot.
         *
         * @tparam IsConst  If @c true, the iterator holds a pointer to a @c const allocator.
         */
        template<bool IsConst>
        class iterator_base
        {
        public:
            using alloc_ptr = std::conditional_t<IsConst, const Derived*, Derived*>;

            // STL iterator traits
            using iterator_category = std::forward_iterator_tag;
            using value_type = index_t;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = index_t;

            iterator_base() noexcept = default;

            /**
             * @brief Constructs an iterator pointing at position @p pos in allocator @p alloc.
             * @param alloc  Pointer to the owning allocator. Must not be null.
             * @param pos    Current index position. Use `capacity()` for the end sentinel.
             */
            iterator_base(alloc_ptr alloc, index_t pos) noexcept
                : m_alloc(alloc)
                , m_pos(pos)
            {
            }

            /**
             * @brief Implicit conversion from non-const to const iterator.
             */
            operator iterator_base<true>() const noexcept
                requires(!IsConst)
            {
                return iterator_base<true>(m_alloc, m_pos);
            }

            /**
             * @brief Returns the current allocated index.
             * @pre The iterator must not be equal to `end()`.
             */
            reference operator*() const noexcept
            {
                return m_pos;
            }

            /**
             * @brief Advances to the next allocated index (pre-increment).
             * @pre The iterator must not be equal to `end()`.
             */
            iterator_base& operator++() noexcept
            {
                m_pos = m_alloc->next_after(m_pos);
                return *this;
            }

            /**
             * @brief Advances to the next allocated index (post-increment).
             * @pre The iterator must not be equal to `end()`.
             */
            iterator_base operator++(int) noexcept
            {
                auto copy = *this;
                m_pos = m_alloc->next_after(m_pos);
                return copy;
            }

            bool operator==(const iterator_base& other) const noexcept
            {
                return m_pos == other.m_pos;
            }

            bool operator!=(const iterator_base& other) const noexcept
            {
                return m_pos != other.m_pos;
            }

        private:
            alloc_ptr m_alloc = nullptr;
            index_t   m_pos = 0;
        };

        using iterator = iterator_base<false>;
        using const_iterator = iterator_base<true>;

    public:
        /**
         * @brief Returns @c true if no indices are currently allocated.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return derived().size() == 0;
        }

        /**
         * @brief Returns @c true if all possible indices are allocated.
         */
        [[nodiscard]] bool full() const noexcept
        {
            return derived().size() == derived().capacity();
        }

        /**
         * @brief Returns the number of indices available for allocation.
         */
        [[nodiscard]] size_t available() const noexcept
        {
            return derived().capacity() - derived().size();
        }

        /**
         * @brief Returns an iterator to the first allocated index.
         *
         * If no indices are allocated, returns @ref end().
         */
        [[nodiscard]] iterator begin() noexcept
        {
            return {&derived(), derived().find_first()};
        }

        /// @copydoc begin()
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return {&derived(), derived().find_first()};
        }

        /// @copydoc begin()
        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return {&derived(), derived().find_first()};
        }

        /**
         * @brief Returns a sentinel iterator one past the last possible index.
         *
         * The sentinel position is equal to `capacity()` and does not correspond
         * to any allocated slot.
         */
        [[nodiscard]] iterator end() noexcept
        {
            return {&derived(), static_cast<index_t>(derived().capacity())};
        }

        /// @copydoc end()
        [[nodiscard]] const_iterator end() const noexcept
        {
            return {&derived(), static_cast<index_t>(derived().capacity())};
        }

        /// @copydoc end()
        [[nodiscard]] const_iterator cend() const noexcept
        {
            return {&derived(), static_cast<index_t>(derived().capacity())};
        }

    private:
        const Derived& derived() const noexcept
        {
            return static_cast<const Derived&>(*this);
        }

        Derived& derived() noexcept
        {
            return static_cast<Derived&>(*this);
        }
    };

} // namespace tavros::core
