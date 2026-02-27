#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/ids/handle_base.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>

namespace tavros::core
{

    template<handle_tagged Tag>
    class handle_allocator final : noncopyable
    {
    public:
        using handle_type = handle_base<Tag>;
        using index_allocator_type = l3_bitmap_index_allocator;
        static constexpr size_t k_capacity = index_allocator_type::k_max_index;

    public:
        handle_allocator() noexcept = default;
        ~handle_allocator() noexcept = default;

        [[nodiscard]] handle_type allocate() noexcept
        {
            auto idx = m_bitmap.allocate();
            TAV_ASSERT(idx != invalid_index);
            if (idx == invalid_index) {
                return {};
            }
            return handle_type(m_gen[idx], static_cast<handle_index_t>(idx));
        }

        bool deallocate(handle_type handle) noexcept
        {
            auto idx = handle.index();
            if (m_bitmap.deallocate(idx)) {
                ++m_gen[idx];
            }
        }

        [[nodiscard]] bool contains(handle_type handle) const noexcept
        {
            auto idx = handle.index();
            return m_bitmap.contains(idx) && m_gen[idx] == handle.generation();
        }

        void reset() noexcept
        {
            m_bitmap.reset();
            std::memset(m_gen, 0, k_capacity * sizeof(handle_gen_t));
        }

        /**
         * @brief Returns the maximum number of handles.
         */
        [[nodiscard]] virtual size_t capacity() const noexcept
        {
            return m_bitmap.capacity();
        }

        /**
         * @brief Returns the number of currently allocated indices.
         */
        [[nodiscard]] virtual size_t size() const noexcept
        {
            return m_bitmap.size();
        }

        /**
         * @brief Returns 'true' if no handle is allocated.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return m_bitmap.empty();
        }

        /**
         * @brief Returns 'true' if the allocator has allocated all possible handles.
         */
        [[nodiscard]] bool full() const noexcept
        {
            return m_bitmap.full();
        }

        /**
         * @brief Returns the number of free handles.
         */
        [[nodiscard]] size_t available() const noexcept
        {
            return m_bitmap.available();
        }

    private:
        l3_bitmap_index_allocator m_bitmap;
        handle_gen_t              m_gen[k_capacity] = {};
    };

} // namespace tavros::core
