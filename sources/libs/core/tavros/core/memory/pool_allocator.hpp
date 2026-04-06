#pragma once

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/memory/raw_ptr.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>
#include <tavros/core/types.hpp>
#include <tavros/core/debug/verify.hpp>

#include <cstddef>
#include <memory>
#include <new>

namespace tavros::core
{

    template<typename T, size_t Capacity>
    class pool_allocator final : noncopyable
    {
        static_assert(Capacity > 0, "Capacity must be > 0");
        static_assert(Capacity <= l3_bitmap_index_allocator::k_capacity, "Capacity exceeds index allocator limit");

    private:
        struct alignas(T) slot_t
        {
            uint8 data[sizeof(T)];
        };

    public:
        // -----------------------------------------------------------------------
        // Iterators
        // -----------------------------------------------------------------------

        /**
         * @brief Forward iterator over live blocks in the pool.
         *
         * Dereferencing yields a pointer to the block:
         * - non-const iterator -> `T*`
         * - const iterator     -> `const T*`
         *
         * @tparam IsConst  If @c true the iterator is read-only.
         */
        template<bool IsConst>
        class iterator_base
        {
            using pool_ptr = std::conditional_t<IsConst, const pool_allocator*, pool_allocator*>;
            using index_iter = std::conditional_t<IsConst, typename l3_bitmap_index_allocator::const_iterator, typename l3_bitmap_index_allocator::iterator>;
            using object_ptr = std::conditional_t<IsConst, const T*, T*>;

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = object_ptr;
            using difference_type = std::ptrdiff_t;
            using reference = object_ptr;
            using pointer = void;

            iterator_base() noexcept = default;

            iterator_base(pool_ptr pool, index_iter it) noexcept
                : m_pool(pool)
                , m_it(it)
            {
            }

            /// @brief Implicit conversion from non-const to const iterator.
            operator iterator_base<true>() const noexcept
                requires(!IsConst)
            {
                return {m_pool, m_it};
            }

            reference operator*() const noexcept
            {
                return m_pool->slot_ptr(*m_it);
            }

            iterator_base& operator++() noexcept
            {
                ++m_it;
                return *this;
            }

            iterator_base operator++(int) noexcept
            {
                auto copy = *this;
                ++m_it;
                return copy;
            }

            bool operator==(const iterator_base& other) const noexcept
            {
                return m_it == other.m_it;
            }

            bool operator!=(const iterator_base& other) const noexcept
            {
                return m_it != other.m_it;
            }

        private:
            pool_ptr   m_pool = nullptr;
            index_iter m_it = {};
        };

        using iterator = iterator_base<false>;
        using const_iterator = iterator_base<true>;

    public:
        pool_allocator() noexcept = default;
        ~pool_allocator() noexcept = default;
        pool_allocator(pool_allocator&&) noexcept = default;
        pool_allocator& operator=(pool_allocator&&) noexcept = default;

        [[nodiscard]] static constexpr size_t capacity() noexcept
        {
            return Capacity;
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return m_index_alloc.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return m_index_alloc.size() == 0;
        }

        [[nodiscard]] bool full() const noexcept
        {
            return m_index_alloc.size() == Capacity;
        }

        [[nodiscard]] T* allocate() noexcept
        {
            index_t idx = m_index_alloc.allocate();
            if (idx == invalid_index) {
                return nullptr;
            }
            return slot_ptr(idx);
        }

        void deallocate(T* ptr) noexcept
        {
            TAV_ASSERT(ptr != nullptr);
            TAV_ASSERT(contains(ptr));
            index_t idx = index_of(ptr);
            TAV_VERIFY(m_index_alloc.deallocate(idx));
        }

        [[nodiscard]] bool contains(const T* ptr) const noexcept
        {
            if (!in_range(ptr)) {
                return false;
            }
            return m_index_alloc.contains(index_of(ptr));
        }

        void reset() noexcept
        {
            m_index_alloc.reset();
        }

        [[nodiscard]] iterator begin() noexcept
        {
            return {this, m_index_alloc.begin()};
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return {this, m_index_alloc.begin()};
        }

        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return {this, m_index_alloc.cbegin()};
        }

        [[nodiscard]] iterator end() noexcept
        {
            return {this, m_index_alloc.end()};
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return {this, m_index_alloc.end()};
        }

        [[nodiscard]] const_iterator cend() const noexcept
        {
            return {this, m_index_alloc.cend()};
        }

    private:
        [[nodiscard]] T* slot_ptr(index_t idx) noexcept
        {
            return std::launder(reinterpret_cast<T*>(m_storage[idx].data));
        }

        [[nodiscard]] const T* slot_ptr(index_t idx) const noexcept
        {
            return std::launder(reinterpret_cast<const T*>(m_storage[idx].data));
        }

        [[nodiscard]] index_t index_of(const T* ptr) const noexcept
        {
            auto base = reinterpret_cast<uintptr_t>(m_storage);
            auto p = reinterpret_cast<uintptr_t>(ptr);
            return static_cast<index_t>((p - base) / sizeof(slot_t));
        }

        [[nodiscard]] bool in_range(const T* ptr) const noexcept
        {
            auto base = reinterpret_cast<uintptr_t>(m_storage);
            auto p = reinterpret_cast<uintptr_t>(ptr);
            auto end = base + Capacity * sizeof(slot_t);
            return p >= base && p < end && ((p - base) % sizeof(slot_t) == 0);
        }

    private:
        slot_t                    m_storage[Capacity];
        l3_bitmap_index_allocator m_index_alloc;
    };

} // namespace tavros::core
