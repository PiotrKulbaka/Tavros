#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/ids/handle_base.hpp>
#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>

namespace tavros::core
{

    /**
     * @brief Allocator for typed handles with generation-based validity tracking.
     *
     * Manages allocation and deallocation of @ref handle_base handles backed by
     * a bitmap index allocator. Each handle carries a generation counter so that
     * stale handles (referring to already-deallocated slots) can be detected via
     * @ref contains.
     *
     * @tparam Tag  Tag type that uniquely identifies the handle type family.
     *              Must satisfy the @ref handle_tagged concept.
     *
     * @note The allocator is non-copyable. Maximum capacity is fixed at compile
     *       time by @ref k_capacity.
     */
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

        handle_type at(index_t idx) const noexcept
        {
            if (m_bitmap.contains(idx)) {
                return handle_type(m_gen[idx], idx);
            }
            return {};
        }

        /**
         * @brief Allocates a new handle.
         *
         * Finds a free slot in the bitmap, marks it as allocated, and returns a
         * handle with the current generation for that slot.
         *
         * @return Valid handle on success, null handle if the allocator is full.
         * @note   Asserts in debug builds if no free slot is available.
         */
        [[nodiscard]] handle_type allocate() noexcept
        {
            auto idx = m_bitmap.allocate();
            TAV_ASSERT(idx != invalid_index);
            if (idx == invalid_index) {
                return {};
            }
            return handle_type(m_gen[idx], static_cast<index_t>(idx));
        }

        /**
         * @brief Deallocates a previously allocated handle.
         *
         * Marks the slot as free and increments the generation counter so that
         * any surviving copies of @p handle become stale and fail @ref contains.
         *
         * @param handle  Handle to deallocate.
         * @return        @c true if the handle was valid and has been freed,
         *                @c false if the handle was already stale or invalid.
         */
        bool deallocate(handle_type handle) noexcept
        {
            if (!contains(handle)) {
                return false;
            }

            auto idx = handle.index();
            m_bitmap.deallocate(idx);
            ++m_gen[idx];
            return true;
        }

        /**
         * @brief Checks whether a handle is currently valid.
         *
         * A handle is valid if its index is allocated and its generation matches
         * the current generation stored for that index.
         *
         * @param handle  Handle to test.
         * @return        @c true if the handle is live, @c false otherwise.
         */
        [[nodiscard]] bool contains(handle_type handle) const noexcept
        {
            auto idx = handle.index();
            return m_bitmap.contains(idx) && m_gen[idx] == handle.generation();
        }

        /**
         * @brief Resets the allocator to its initial state.
         *
         * Frees all allocated slots and zeroes all generation counters.
         * All previously issued handles become invalid.
         */
        void reset() noexcept
        {
            m_bitmap.reset();
            std::memset(m_gen, 0, k_capacity * sizeof(handle_gen_t));
        }

        /**
         * @brief Returns the maximum number of handles.
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
            return m_bitmap.capacity();
        }

        /**
         * @brief Returns the number of currently allocated indices.
         */
        [[nodiscard]] size_t size() const noexcept
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
