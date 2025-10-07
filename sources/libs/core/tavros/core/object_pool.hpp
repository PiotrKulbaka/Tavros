#pragma once

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/verify.hpp>
#include <tavros/core/math/bitops.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>
#include <tavros/core/object_handle.hpp>

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
    template<class T>
    class object_pool : noncopyable
    {
    private:
        using idx_alc_t = l3_bitmap_index_allocator;

    public:
        using handle_type = object_handle<T>;

        static_assert(idx_alc_t::k_max_index <= 0x00ffffffu, "index allocator supports more indices than 24 bits");
        static_assert(std::is_nothrow_move_constructible_v<T>, "object_pool requires noexcept move for exception-safety when expanding");

    public:
        /**
         * @brief Construct a object pool with a memory allocator.
         * @param alc Memory allocator to use for internal allocations.
         */
        object_pool(allocator* alc)
            : m_mem_alc(alc)
        {
            TAV_VERIFY(m_mem_alc);
        }

        /**
         * @brief Destructor destroys all allocated objects and frees memory.
         */
        ~object_pool()
        {
            // m_idx_alc can be in dirty state and m_res can be nullptr, because this object was moved
            if (m_res) {
                for (index_type i = 0; i <= m_max_idx; ++i) {
                    if (m_idx_alc.allocated(i)) {
                        std::destroy_at(m_res + i);
                    }
                }
            }

            if (m_mem) {
                m_mem_alc->deallocate(m_mem);
            }
        }

        /**
         * @brief Move constructor, transfers ownership from other pool.
         */
        object_pool(object_pool&& other) noexcept
            : m_mem_alc(other.m_mem_alc)
            , m_idx_alc(other.m_idx_alc)
            , m_max_idx(other.m_max_idx)
            , m_mem(other.m_mem)
            , m_gen(other.m_gen)
            , m_res(other.m_res)
            , m_capacity(other.m_capacity)
            , m_size(other.m_size)

        {
            // If the verification failed, the object has already been moved.
            TAV_VERIFY(other.m_mem_alc);

            other.m_mem_alc = nullptr;
            // m_idx_alc - not needed to reset
            other.m_max_idx = 0;
            other.m_mem = nullptr;
            other.m_gen = nullptr;
            other.m_res = nullptr;
            other.m_capacity = 0;
            other.m_size = 0;
        }

        /**
         * @brief Move assignment operator, transfers ownership from other pool.
         */
        object_pool& operator=(object_pool&& other) noexcept
        {
            // If the verification failed, the object has already been moved.
            TAV_VERIFY(other.m_mem_alc);

            if (this != &other) {
                if (m_mem) {
                    // Destroy existing objects and free memory
                    for (index_type i = 0; i <= m_max_idx; ++i) {
                        if (m_idx_alc.allocated(i)) {
                            std::destroy_at(m_res + i);
                        }
                    }
                    m_mem_alc->deallocate(m_mem);
                }

                // Move data
                m_mem_alc = other.m_mem_alc;
                m_idx_alc = other.m_idx_alc;
                m_max_idx = other.m_max_idx;
                m_mem = other.m_mem;
                m_gen = other.m_gen;
                m_res = other.m_res;
                m_capacity = other.m_capacity;
                m_size = other.m_size;

                // Reset other
                other.m_mem_alc = nullptr;
                // other.m_idx_alc
                other.m_max_idx = 0;
                other.m_mem = nullptr;
                other.m_gen = nullptr;
                other.m_res = nullptr;
                other.m_capacity = 0;
                other.m_size = 0;
            }
            return *this;
        }

        /**
         * @brief Iterate over all allocated objects and apply a function.
         * @param fun Function to call for each handle and object reference.
         */
        template<typename Fn>
        void for_each(Fn&& fun)
        {
            for (index_type i = 0; i <= m_max_idx; ++i) {
                if (m_idx_alc.allocated(i)) {
                    handle_type h = make_handle(i);
                    T&          res = m_res[i];
                    fun(h, res);
                }
            }
        }

        /**
         * @brief Returns the number of currently allocated objects.
         */
        [[nodiscard]] size_t size() const noexcept
        {
            return m_size;
        }

        /**
         * @brief Checks whether a object with the given handle currently exists in the pool.
         * @param h The handle of the object to check.
         * @return True if the object exists and is currently allocated; false otherwise.
         */
        [[nodiscard]] bool exists(handle_type h) const noexcept
        {
            auto idx = extract_index(h.id);
            if (idx < m_capacity && (extract_gen(h.id) == current_gen(idx))) {
                return m_idx_alc.allocated(idx);
            }
            return false;
        }

        /**
         * @brief Try to get a pointer to a object by handle.
         * @param h object handle.
         * @return Pointer to object or nullptr if invalid.
         */
        [[nodiscard]] T* try_get(handle_type h) noexcept
        {
            return const_cast<T*>(static_cast<const object_pool*>(this)->try_get(h));
        }

        /**
         * @brief Try to get a const pointer to a object by handle.
         * @param h object handle.
         * @return Const pointer to object or nullptr if invalid.
         */
        [[nodiscard]] const T* try_get(handle_type h) const noexcept
        {
            TAV_ASSERT(m_capacity != 0);
            if (m_capacity == 0) {
                return nullptr;
            }

            auto idx = extract_index(h.id);
            TAV_ASSERT(idx < m_capacity);

            if (idx >= m_capacity) {
                return nullptr;
            }

            auto gen = extract_gen(h.id);
            auto cur = current_gen(idx);

            TAV_ASSERT(gen == cur);

            if (gen != cur) {
                return nullptr;
            }

            if (!m_idx_alc.allocated(idx)) {
                return nullptr;
            }

            return m_res + idx;
        }

        /**
         * @brief Add a new object by moving it into the pool.
         * @param res object to move.
         * @return Handle to the newly added object.
         */
        [[nodiscard]] handle_type add(T&& res)
        {
            auto idx = m_idx_alc.allocate();
            TAV_ASSERT(idx != invalid_index);

            if (idx == invalid_index) {
                return handle_type::invalid();
            }

            // Should be called before update m_max_idx
            ensure_allocation(idx);

            if (m_max_idx < idx) {
                m_max_idx = idx;
            }

            // m_res[idx] = std::move(res); - UB; instead used placement new
            std::construct_at(m_res + idx, std::move(res));
            ++m_size;

            return make_handle(idx);
        }

        /**
         * @brief Construct and add a new object in-place.
         * @tparam Args Constructor argument types.
         * @param args Arguments to forward to T constructor.
         * @return Handle to the newly added object.
         */
        template<typename... Args>
        [[nodiscard]] handle_type emplace_add(Args&&... args)
        {
            auto idx = m_idx_alc.allocate();
            TAV_ASSERT(idx != invalid_index);

            if (idx == invalid_index) {
                return handle_type::invalid();
            }

            // Should be called before update m_max_idx
            ensure_allocation(idx);

            if (m_max_idx < idx) {
                m_max_idx = idx;
            }

            // Don't use construct_at here because it may not work in some cases
            // std::construct_at(m_res + idx, std::forward<Args>(args)...);
            new (m_res + idx) T(std::forward<Args>(args)...);
            ++m_size;

            return make_handle(idx);
        }

        /**
         * @brief Erase a object from the pool.
         * @param h object handle.
         * @return True if object was successfully erased.
         */
        bool erase(handle_type h)
        {
            auto idx = extract_index(h.id);
            auto gen = extract_gen(h.id);
            auto cur = current_gen(idx);

            if (gen != cur) {
                return false;
            }

            auto success = m_idx_alc.try_deallocate(idx);
            if (!success) {
                return false;
            }

            if (m_max_idx == idx && idx > 0) {
                index_type i = idx - 1;
                while (i > 0) {
                    if (m_idx_alc.allocated(i)) {
                        break;
                    }
                    --i;
                }
                m_max_idx = i;
            }

            increase_gen(idx);
            std::destroy_at(m_res + idx);
            --m_size;

            return true;
        }

        /**
         * @brief Clear all objects from the pool.
         * Resets internal index allocator and size counter.
         */
        void clear()
        {
            for (index_type i = 0; i <= m_max_idx; ++i) {
                if (m_idx_alc.allocated(i)) {
                    m_res[i].~T();
                }
            }
            m_max_idx = 0;
            m_idx_alc.reset();
            m_size = 0;
        }

    private:
        void ensure_allocation(index_type idx)
        {
            auto new_capacity = adapt_capacity(static_cast<size_t>(idx) + 1);
            if (m_capacity < new_capacity) {
                // gen size + res size + aignment for res size
                size_t new_gen_size = new_capacity;
                size_t required_size = new_gen_size + new_capacity * sizeof(T) + alignof(T);

                void* new_mem = m_mem_alc->allocate(required_size, 8, "object_pool");
                TAV_ASSERT(new_mem);

                uint8* new_gen = reinterpret_cast<uint8*>(new_mem);
                auto   res_addr = math::align_up(reinterpret_cast<size_t>(new_mem) + new_gen_size, alignof(T));
                T*     new_res = reinterpret_cast<T*>(res_addr);

                if (m_mem != nullptr) {
                    move_to_new_memory(new_gen, new_res, new_capacity);
                    m_mem_alc->deallocate(m_mem);
                } else {
                    std::fill(new_gen, new_gen + new_capacity, 0);
                }

                m_mem = new_mem;
                m_gen = new_gen;
                m_res = new_res;
                m_capacity = new_capacity;
            }
        }

        size_t adapt_capacity(size_t capacity) noexcept
        {
            if (capacity < 2) {
                return 2;
            }
            size_t adapted = static_cast<size_t>(math::ceil_power_of_two(capacity));
            size_t max_idx = static_cast<size_t>(m_idx_alc.max_index());

            if (adapted > max_idx) {
                return max_idx;
            }
            return adapted;
        }

        void move_to_new_memory(uint8* new_gen, T* new_objects, size_t new_capacity)
        {
            // Move gen
            std::memcpy(new_gen, m_gen, m_capacity);
            std::fill(new_gen + m_capacity, new_gen + new_capacity, 0);

            // Move objects
            for (index_type i = 0; i <= m_max_idx; ++i) {
                if (m_idx_alc.allocated(i)) {
                    // Move to new object
                    new (new_objects + i) T(std::move(m_res[i]));
                    // Deallocate old object
                    std::destroy_at(m_res + i);
                }
            }
        }

        handle_type make_handle(index_type idx) const
        {
            uint32 gen = static_cast<uint32>(current_gen(idx));
            return {(gen << 24) | idx};
        }

        uint8 current_gen(index_type idx) const
        {
            return m_gen[idx] & 0x7f;
        }

        void increase_gen(index_type idx)
        {
            ++m_gen[idx];
        }

        static index_type extract_index(uint32 h_id)
        {
            return h_id & 0x00ffffff;
        }

        static uint8 extract_gen(uint32 h_id)
        {
            return static_cast<uint8>((h_id >> 24) & 0x7f);
        }

    private:
        allocator* m_mem_alc = nullptr; // Dynamic memory allocator
        idx_alc_t  m_idx_alc;           // TODO: upgrade to dynamic index allocator
        index_type m_max_idx = 0;       // Optimization for loops

        void*  m_mem = nullptr;         // memory pointer
        uint8* m_gen = nullptr;         // generation
        T*     m_res = nullptr;         // objects
        size_t m_capacity = 0;          // current capacity of m_objects and m_gen
        size_t m_size = 0;
    };

} // namespace tavros::core
