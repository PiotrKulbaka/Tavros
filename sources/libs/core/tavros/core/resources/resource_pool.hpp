#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/math/bitops.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>

namespace tavros::core
{

    template<class T>
    class resource_pool
    {
    private:
        using idx_alc_t = l3_bitmap_index_allocator;

    public:
        using handle_t = uint32;

        static constexpr handle_t invalid_handle = 0xffffffffui32;

    public:
        resource_pool()
        {
            std::fill(m_gen, m_gen + idx_alc_t::k_max_index, 1);

            m_allocator = new mallocator();
        }

        ~resource_pool()
        {
            for (index_type i = 0; i <= m_max_idx; ++i) {
                if (m_index_allocator.allocated(i)) {
                    m_resources[i].~T();
                }
            }

            if (m_resources) {
                m_allocator->deallocate(m_resources);
            }

            delete m_allocator;
        }

        /*struct resource_item
        {
            handle_t handle;
            T& resource;
        };

        struct resource_item_iterator
        {
            resource_pool* pool;
            size_t index;

            bool operator!=(const iterator& other) const { return index != other.index; }

            resource_item_iterator& operator++()
            {
                ++index;
                return *this;
            }

            resource_item operator*() const
            {
                return { pool->m_dense_handles[index], pool->m_dense[index] };
            }
        };

        struct items_range
        {
            resource_pool* pool;

            resource_item_iterator begin()
            {
                return {pool, 0};
            }

            resource_item_iterator end()
            {
                return { pool, pool->m_dense.size() };
            }
        };

        items_range items() { return { this }; }*/

        template<typename Fn>
        void for_each(Fn&& fun)
        {
            for (index_type i = 0; i <= m_max_idx; ++i) {
                if (m_index_allocator.allocated(i)) {
                    handle_t h = make_handle(i);
                    T&       res = m_resources[i];
                    fun(h, res);
                }
            }
        }

        size_t size() const noexcept
        {
            return m_size;
        }

        T* try_get(handle_t h)
        {
            auto idx = extract_index(h);
            TAV_ASSERT(idx < m_index_allocator.max_index());

            if (idx >= m_index_allocator.max_index()) {
                return nullptr;
            }

            auto gen = extract_gen(h);
            auto cur = current_gen(idx);

            if (gen != cur || !m_index_allocator.allocated(idx)) {
                return nullptr;
            }

            return m_resources + idx;
        }

        const T* try_get(handle_t h) const
        {
            return try_get(h);
        }

        [[nodiscard]] handle_t add(T&& res)
        {
            auto idx = m_index_allocator.allocate();
            TAV_ASSERT(idx != invalid_index);

            if (idx == invalid_index) {
                return invalid_handle;
            }

            // Should be called before update m_max_idx
            ensure_allocation(idx);

            if (m_max_idx < idx) {
                m_max_idx = idx;
            }

            ++m_size;
            m_resources[idx] = std::move(res);
            return make_handle(idx);
        }

        template<typename... Args>
        [[nodiscard]] handle_t emplace_add(Args&&... args)
        {
            auto idx = m_index_allocator.allocate();
            TAV_ASSERT(idx != invalid_index);

            if (idx == invalid_index) {
                return invalid_handle;
            }

            // Should be called before update m_max_idx
            ensure_allocation(idx);

            if (m_max_idx < idx) {
                m_max_idx = idx;
            }

            ++m_size;
            new (m_resources + idx) T(std::forward<Args>(args)...);
            return make_handle(idx);
        }

        bool erase(handle_t h)
        {
            auto idx = extract_index(h);
            auto gen = extract_gen(h);
            auto cur = current_gen(idx);

            TAV_ASSERT(gen == cur);

            if (gen != cur) {
                return false;
            }

            auto success = m_index_allocator.try_deallocate(idx);
            if (!success) {
                return false;
            }

            increase_gen(idx);
            m_resources[idx].~T();
            --m_size;

            return true;
        }

        void clear()
        {
            for (index_type i = 0; i <= m_max_idx; ++i) {
                if (m_index_allocator.allocated(i)) {
                    m_resources[i].~T();
                }
            }
            std::fill(m_gen, m_gen + idx_alc_t::k_max_index, 1);
            m_max_idx = 0;
            m_index_allocator.reset();
            m_size = 0;
        }

    private:
        void ensure_allocation(index_type idx)
        {
            auto needed_capacity = adapt_capacity(static_cast<size_t>(idx) + 1);
            if (m_current_capacity < needed_capacity) {
                size_t required_size = needed_capacity * sizeof(T);
                T*     new_res = reinterpret_cast<T*>(m_allocator->allocate(required_size, alignof(T), "resource_pool"));

                if (m_resources != nullptr) {
                    move_resources_to_new_memory(new_res);
                    m_allocator->deallocate(m_resources);
                }

                m_resources = new_res;
                m_current_capacity = needed_capacity;
            }
        }

        size_t adapt_capacity(size_t capacity) noexcept
        {
            if (capacity < 2) {
                return 2;
            }
            size_t adapted = static_cast<size_t>(math::ceil_power_of_two(capacity));
            size_t max_idx = static_cast<size_t>(m_index_allocator.max_index());

            if (adapted > max_idx) {
                return max_idx;
            }
            return adapted;
        }

        void move_resources_to_new_memory(T* new_resources)
        {
            for (index_type i = 0; i <= m_max_idx; ++i) {
                if (m_index_allocator.allocated(i)) {
                    // Move to new resource
                    new (new_resources + i) T(std::move(m_resources[i]));
                    // Deallocate old resource
                    m_resources[i].~T();
                }
            }
        }

        handle_t make_handle(index_type idx)
        {
            uint32 gen = static_cast<uint32>(current_gen(idx));
            return (gen << 24) | idx;
        }

        uint8 current_gen(index_type idx)
        {
            return m_gen[idx] & 0x7f;
        }

        void increase_gen(index_type idx)
        {
            ++m_gen[idx];
        }

        static index_type extract_index(handle_t h)
        {
            return h & 0x00ffffff;
        }

        static uint8 extract_gen(handle_t h)
        {
            return static_cast<uint8>((h >> 24) & 0x7f);
        }

    private:
        allocator* m_allocator;
        idx_alc_t  m_index_allocator;
        uint8      m_gen[idx_alc_t::k_max_index]; // generation

        index_type m_max_idx = 0;

        T*     m_resources = nullptr;
        size_t m_current_capacity = 0; // current capacity of m_resources
        size_t m_size = 0;
    };

} // namespace tavros::core
