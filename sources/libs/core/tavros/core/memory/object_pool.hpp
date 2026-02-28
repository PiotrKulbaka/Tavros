#pragma once

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/verify.hpp>
#include <tavros/core/math/bitops.hpp>
#include <tavros/core/memory/raw_ptr.hpp>
#include <tavros/core/ids/handle_allocator.hpp>

#include <type_traits>

namespace tavros::core
{

    /**
     * @brief Pool that manages objects of type T with stable handles.
     *
     * Objects are stored in contiguous memory, and handles are used to access them.
     * Moving the pool does not invalidate handles for the moved objects.
     *
     * The pool allocates memory in powers-of-two sizes to efficiently grow
     * and always keeps objects in a single contiguous block of memory,
     * minimizing fragmentation. At any given time, the pool uses at most
     * one contiguous dynamic memory block. Full defragmentation is the
     * responsibility of the allocator provided to the pool.
     *
     * The type T must be nothrow-move-constructible, because objects
     * may be relocated during memory expansion.
     *
     * Accessing objects is extremely fast, typically O(1), except in rare
     * cases when memory expansion and object relocation occur.
     */
    template<class T, handle_tagged Tag>
    class object_pool final : noncopyable
    {
    public:
        using handle_type = handle_base<Tag>;
        using handle_allocator_type = handle_allocator<Tag>;

        static_assert(handle_allocator_type::k_capacity < 0xffffffffu, "index allocator supports more indices than 32 bits");
        static_assert(std::is_nothrow_move_constructible_v<T>, "object_pool requires noexcept move for exception-safety when expanding");

    public:
        /**
         * @brief Construct a object pool with a memory allocator.
         * @param alc Memory allocator to use for internal allocations.
         */
        object_pool(allocator* alc) noexcept
            : m_mem_alc(alc)
        {
            TAV_VERIFY(alc);
        }

        /**
         * @brief Destructor destroys all allocated objects and frees memory.
         */
        ~object_pool() noexcept
        {
            if (m_mem) {
                destroy_all();
                m_mem_alc->deallocate(m_mem.get());
            }
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
         * @brief Iterate over all allocated objects and apply a function.
         * @param fun Function to call for each handle and object reference.
         */
        template<typename Func>
        void for_each(Func&& func)
        {
            for (index_t i = 0; i < m_end_idx; ++i) {
                auto h = m_h_alc.at(i);
                if (h) {
                    func(h, m_res.get()[i]);
                }
            }
        }

        /**
         * @brief Returns the number of currently allocated objects.
         */
        [[nodiscard]] size_t size() const noexcept
        {
            return m_h_alc.size();
        }

        /**
         * @brief Returns @c true if no objects are currently allocated.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return m_h_alc.empty();
        }

        /**
         * @brief Returns the current capacity of the internal storage (in number of objects).
         *
         * Capacity grows automatically on @ref push or @ref emplace when needed.
         * Use @ref reserve to pre-allocate.
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
            return m_capacity;
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
            return m_h_alc.contains(handle);
        }

        /**
         * @brief Returns a pointer to the object identified by @p handle, or @c nullptr if invalid.
         * @param handle Handle to look up.
         * @return Pointer to the object, or @c nullptr if the handle is stale or invalid.
         */
        [[nodiscard]] T* find(handle_type handle) noexcept
        {
            return m_h_alc.contains(handle) ? m_res.get() + handle.index() : nullptr;
        }

        /**
         * @brief Returns a const pointer to the object identified by @p handle, or @c nullptr if invalid.
         * @param handle Handle to look up.
         * @return Const pointer to the object, or @c nullptr if the handle is stale or invalid.
         */
        [[nodiscard]] const T* find(handle_type handle) const noexcept
        {
            return m_h_alc.contains(handle) ? m_res.get() + handle.index() : nullptr;
        }

        /**
         * @brief Inserts a copy of @p res into the pool.
         *
         * May trigger memory expansion and relocation of all existing objects.
         *
         * @param res Object to copy.
         * @return Handle to the newly inserted object, or a null handle if the pool is full.
         */
        [[nodiscard]] handle_type push(const T& res)
        {
            return emplace(res);
        }

        /**
         * @brief Moves @p res into the pool.
         *
         * May trigger memory expansion and relocation of all existing objects.
         *
         * @param res Object to move.
         * @return Handle to the newly inserted object, or a null handle if the pool is full.
         */
        [[nodiscard]] handle_type push(T&& res)
        {
            return emplace(std::move(res));
        }

        /**
         * @brief Constructs an object in-place inside the pool.
         *
         * Arguments are forwarded directly to the constructor of T.
         * May trigger memory expansion and relocation of all existing objects.
         *
         * @tparam Args Constructor argument types.
         * @param args  Arguments forwarded to `T(args...)`.
         * @return Handle to the newly constructed object, or a null handle if the pool is full.
         */
        template<typename... Args>
        [[nodiscard]] handle_type emplace(Args&&... args)
        {
            auto h = m_h_alc.allocate();
            if (h) {
                auto idx = h.index();
                ensure_allocation(idx);

                if (m_end_idx <= idx) {
                    m_end_idx = idx + 1;
                }

                std::construct_at(m_res.get() + idx, std::forward<Args>(args)...);
            }

            return h;
        }

        /**
         * @brief Removes the object identified by @p handle from the pool.
         *
         * The handle becomes invalid immediately. Any other copies of the handle
         * will also be considered stale.
         *
         * @param handle Handle of the object to erase.
         * @return @c true if the object was found and erased; @c false if the handle was invalid.
         */
        bool erase(handle_type handle)
        {
            if (m_h_alc.deallocate(handle)) {
                auto idx = handle.index();
                std::destroy_at(m_res.get() + idx);

                if (m_end_idx == idx + 1) {
                    while (m_end_idx > 0 && !m_h_alc.at(m_end_idx - 1)) {
                        --m_end_idx;
                    }
                }

                return true;
            }
            return false;
        }

        /**
         * @brief Destroys all objects and resets the pool to an empty state.
         *
         * Allocated memory is retained. All previously issued handles become invalid.
         */
        void clear()
        {
            destroy_all();
            m_h_alc.reset();
            m_end_idx = 0;
        }

    private:
        void destroy_all() noexcept
        {
            if (m_res) {
                for (index_t i = 0; i < m_end_idx; ++i) {
                    if (m_h_alc.at(i)) {
                        std::destroy_at(m_res.get() + i);
                    }
                }
            }
        }

        void ensure_allocation(index_t idx)
        {
            auto new_capacity = adapt_capacity(static_cast<size_t>(idx) + 1);
            if (m_capacity >= new_capacity) {
                return;
            }

            auto  required_size = new_capacity * sizeof(T) + alignof(T);
            auto* new_mem = static_cast<uint8*>(m_mem_alc->allocate(required_size, 8, "object_pool"));
            TAV_ASSERT(new_mem);

            auto res_addr = math::align_up(reinterpret_cast<size_t>(new_mem), alignof(T));
            T*   new_res = reinterpret_cast<T*>(res_addr);

            if (m_mem != nullptr) {
                for (index_t i = 0; i < m_end_idx; ++i) {
                    if (m_h_alc.at(i)) {
                        new (new_res + i) T(std::move(m_res.get()[i]));
                        std::destroy_at(m_res.get() + i);
                    }
                }
                m_mem_alc->deallocate(m_mem.get());
            }

            m_mem = new_mem;
            m_res = new_res;
            m_capacity = new_capacity;
        }

        size_t adapt_capacity(size_t capacity) noexcept
        {
            if (capacity < 2) {
                return 2;
            }
            auto adapted = static_cast<size_t>(math::ceil_power_of_two(capacity));
            auto max_capacity = static_cast<size_t>(m_h_alc.capacity());
            return adapted < max_capacity ? adapted : max_capacity;
        }

    private:
        raw_ptr<allocator>    m_mem_alc;
        handle_allocator_type m_h_alc;
        index_t               m_end_idx = 0;

        raw_ptr<uint8> m_mem;
        raw_ptr<T>     m_res;
        size_t         m_capacity = 0;
    };

} // namespace tavros::core
