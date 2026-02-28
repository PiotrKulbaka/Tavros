#pragma once

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/verify.hpp>
#include <tavros/core/math/bitops.hpp>
#include <tavros/core/memory/raw_ptr.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/ids/handle_allocator.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>

#include <type_traits>

namespace tavros::core
{

    /**
     * @brief Pool that manages objects of type T with stable handles.
     *
     * Objects are stored in contiguous memory indexed by handle slots.
     * Handles remain valid across memory expansions - moving the pool
     * does not invalidate any issued handle.
     *
     * Storage grows in powers of two. At any given time a single contiguous
     * block is used, minimising fragmentation.
     *
     * @par Complexity
     * - `emplace` / `push` - O(1) amortized; O(n) on expansion (moves all live objects)
     * - `erase`            - O(1)
     * - `find`             - O(1)
     * - iteration          - O(allocated slots)
     *
     * @par Requirements
     * T must be nothrow-move-constructible so that objects can be safely
     * relocated when the storage buffer is expanded.
     *
     * @tparam T    Type of managed objects.
     * @tparam Tag  Handle tag type satisfying @ref handle_tagged.
     */
    template<class T, handle_tagged Tag>
    class object_pool final : noncopyable
    {
    public:
        using handle_type = handle_base<Tag>;
        using handle_allocator_type = handle_allocator<Tag, l3_bitmap_index_allocator>;

        static_assert(handle_allocator_type::k_capacity < 0xffffffffu, 
            "index allocator supports more indices than 32 bits");
        static_assert(std::is_nothrow_move_constructible_v<T>,
            "object_pool requires noexcept move for exception-safety when expanding");

    public:
        /**
         * @brief Forward iterator over live objects in the pool.
         *
         * Dereferencing yields `std::pair<handle_type, T*>` — the handle and
         * a pointer to the corresponding object.
         *
         * @tparam IsConst  If @c true, the pointer in the pair is @c const T*.
         */
        template<bool IsConst>
        class iterator_base
        {
            using pool_ptr = std::conditional_t<IsConst, const object_pool*, object_pool*>;
            using handle_iter = std::conditional_t<IsConst, typename handle_allocator_type::const_iterator, typename handle_allocator_type::iterator>;
            using object_ptr = std::conditional_t<IsConst, const T*, T*>;

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<handle_type, object_ptr>;
            using difference_type = std::ptrdiff_t;
            using reference = value_type;
            using pointer = void;

            iterator_base() noexcept = default;

            iterator_base(pool_ptr pool, handle_iter it) noexcept
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
                auto h = *m_it;
                return {h, m_pool->slot_ptr(h.index())};
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
            pool_ptr    m_pool = nullptr;
            handle_iter m_it = {};
        };

        using iterator = iterator_base<false>;
        using const_iterator = iterator_base<true>;

    public:
        /**
         * @brief Construct an object pool.
         */
        object_pool() noexcept = default;

        /**
         * @brief Destroys all live objects and releases storage.
         */
        ~object_pool() noexcept
        {
            destroy_all();
        }

        /**
         * @brief Move constructor, transfers ownership from other pool.
         */
        object_pool(object_pool&& other) noexcept = default;

        /**
         * @brief Move assignment operator, transfers ownership from other pool.
         */
        object_pool& operator=(object_pool&& other) noexcept = default;

        /**
         * @brief Returns the number of currently allocated objects.
         */
        [[nodiscard]] size_t size() const noexcept
        {
            return m_handle_alloc.size();
        }

        /**
         * @brief Returns @c true if no objects are currently allocated.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return m_handle_alloc.empty();
        }

        /**
         * @brief Returns the current capacity of the internal storage (in number of objects).
         *
         * Capacity grows automatically on @ref push or @ref emplace when needed.
         * Use @ref reserve to pre-allocate.
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
            return m_storage.capacity();
        }

        /**
         * @brief Pre-allocates storage for at least @p count objects.
         *
         * Has no effect if current capacity is already sufficient.
         * Does not construct any objects.
         *
         * @param count Desired minimum capacity.
         */
        void reserve(size_t count)
        {
            if (count > 0) {
                ensure_allocation(static_cast<index_t>(count - 1));
            }
        }

        /**
         * @brief Returns @c true if @p handle refers to a currently live object.
         * @param handle Handle to check.
         */
        [[nodiscard]] bool contains(handle_type handle) const noexcept
        {
            return m_handle_alloc.contains(handle);
        }

        /**
         * @brief Returns a pointer to the object identified by @p handle, or @c nullptr if invalid.
         */
        [[nodiscard]] T* find(handle_type handle) noexcept
        {
            return m_handle_alloc.contains(handle) ? slot_ptr(handle.index()) : nullptr;
        }

        /**
         * @brief Returns a const pointer to the object identified by @p handle, or @c nullptr if invalid.
         */
        [[nodiscard]] const T* find(handle_type handle) const noexcept
        {
            return m_handle_alloc.contains(handle) ? slot_ptr(handle.index()) : nullptr;
        }

        /**
         * @brief Copies @p res into the pool.
         * @return Handle to the inserted object, or null if the pool is full.
         */
        [[nodiscard]] handle_type push(const T& res)
        {
            return emplace(res);
        }

        /**
         * @brief Moves @p res into the pool.
         * @return Handle to the inserted object, or null if the pool is full.
         */
        [[nodiscard]] handle_type push(T&& res)
        {
            return emplace(std::move(res));
        }

        /**
         * @brief Constructs an object in-place inside the pool.
         *
         * If storage expansion or construction throws, the allocated handle
         * is released and the exception is re-thrown. The pool remains valid.
         *
         * @return Handle to the constructed object, or null if the pool is full.
         */
        template<typename... Args>
        [[nodiscard]] handle_type emplace(Args&&... args)
        {
            auto h = m_handle_alloc.allocate();
            if (!h) {
                return {};
            }

            auto idx = h.index();
            try {
                ensure_allocation(idx);
                std::construct_at(slot_ptr(h.index()), std::forward<Args>(args)...);
            } catch (...) {
                m_handle_alloc.deallocate(h);
                throw;
            }

            return h;
        }

        /**
         * @brief Destroys the object identified by @p handle and frees its slot.
         *
         * All copies of @p handle become stale immediately.
         *
         * @return @c true if erased, @c false if the handle was invalid.
         */
        bool erase(handle_type handle)
        {
            if (m_handle_alloc.deallocate(handle)) {
                std::destroy_at(slot_ptr(handle.index()));
                return true;
            }
            return false;
        }

        /**
         * @brief Destroys all objects and resets the pool to an empty state.
         *
         * Storage capacity is retained. All issued handles become invalid.
         */
        void clear()
        {
            destroy_all();
            m_handle_alloc.reset();
        }

        [[nodiscard]] iterator begin() noexcept
        {
            return {this, m_handle_alloc.begin()};
        }

        [[nodiscard]] iterator end() noexcept
        {
            return {this, m_handle_alloc.end()};
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return {this, m_handle_alloc.begin()};
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return {this, m_handle_alloc.end()};
        }

    private:
        void destroy_all() noexcept
        {
            for (auto h : m_handle_alloc) {
                std::destroy_at(slot_ptr(h.index()));
            }
        }

        void ensure_allocation(index_t idx)
        {
            if (idx < m_storage.size()) {
                return;
            }

            size_t required = static_cast<size_t>(idx + 1);
            auto max_capacity = static_cast<size_t>(m_handle_alloc.capacity());
            size_t new_capacity = required <= 2 ? 2 : std::min(static_cast<size_t>(math::ceil_power_of_two(required)), max_capacity);
            m_storage.resize(new_capacity);
        }

    private:
        struct alignas(T) slot_t
        {
            uint8 data[sizeof(T)];
        };

        T* slot_ptr(index_t idx) noexcept
        {
            return std::launder(reinterpret_cast<T*>(m_storage[idx].data));
        }

        const T* slot_ptr(index_t idx) const noexcept
        {
            return std::launder(reinterpret_cast<const T*>(m_storage[idx].data));
        }

    private:
        handle_allocator_type m_handle_alloc;
        vector<slot_t>        m_storage;
    };

} // namespace tavros::core
