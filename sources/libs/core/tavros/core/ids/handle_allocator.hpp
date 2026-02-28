#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/ids/handle_base.hpp>
#include <tavros/core/ids/index_allocator.hpp>

namespace tavros::core
{

    /**
     * @brief Allocator for typed handles with generation-based validity tracking.
     *
     * Each handle carries a generation counter so that stale handles
     * (referring to already-deallocated slots) can be detected via @ref contains.
     *
     * @tparam Tag        Tag type satisfying @ref handle_tagged.
     * @tparam IndexAlloc Underlying index allocator satisfying @ref index_allocator_concept.
     */
    template<handle_tagged Tag, index_allocator_concept IndexAlloc>
    class handle_allocator final : noncopyable
    {
    public:
        using handle_type = handle_base<Tag>;
        using index_allocator_type = IndexAlloc;
        static constexpr size_t k_capacity = index_allocator_type::k_capacity;

    public:
        /**
         * @brief Forward iterator over currently allocated handles.
         * @tparam IsConst  If @c true, owns a pointer to a @c const allocator.
         */
        template<bool IsConst>
        class iterator_base
        {
            using alloc_ptr = std::conditional_t<IsConst, const handle_allocator*, handle_allocator*>;
            using index_iter = std::conditional_t<IsConst, typename index_allocator_type::const_iterator, typename index_allocator_type::iterator>;

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = handle_type;
            using difference_type = std::ptrdiff_t;
            using reference = handle_type;
            using pointer = void;

            iterator_base() noexcept = default;

            iterator_base(alloc_ptr alloc, index_iter it) noexcept
                : m_alloc(alloc)
                , m_it(it)
            {
            }

            /// @brief Implicit conversion from non-const to const iterator.
            operator iterator_base<true>() const noexcept
                requires(!IsConst)
            {
                return {m_alloc, m_it};
            }

            /// @brief Returns the handle for the current slot.
            reference operator*() const noexcept
            {
                const index_t idx = *m_it;
                return handle_type(m_alloc->m_gen[idx], idx);
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
            alloc_ptr  m_alloc = nullptr;
            index_iter m_it = {};
        };

        using iterator = iterator_base<false>;
        using const_iterator = iterator_base<true>;

    public:
        handle_allocator() noexcept = default;

        ~handle_allocator() noexcept = default;

        /**
         * @brief Allocates a new handle.
         * @return Valid handle on success, null handle if the allocator is full.
         */
        [[nodiscard]] handle_type allocate() noexcept
        {
            auto idx = m_index_alloc.allocate();
            TAV_ASSERT(idx != invalid_index);
            if (idx == invalid_index) {
                return {};
            }
            return handle_type(m_gen[idx], static_cast<index_t>(idx));
        }

        /**
         * @brief Deallocates a handle and increments its slot generation.
         *
         * Any surviving copies of @p handle will fail @ref contains after this call.
         *
         * @return @c true if the handle was live and has been freed, @c false otherwise.
         */
        bool deallocate(handle_type handle) noexcept
        {
            if (!contains(handle)) {
                return false;
            }

            auto idx = handle.index();
            m_index_alloc.deallocate(idx);
            ++m_gen[idx];
            return true;
        }

        /**
         * @brief Returns @c true if @p handle refers to a currently live slot.
         *
         * Checks both that the index is allocated and that the generation matches.
         */
        [[nodiscard]] bool contains(handle_type handle) const noexcept
        {
            auto idx = handle.index();
            return m_index_alloc.contains(idx) && m_gen[idx] == handle.generation();
        }

        /**
         * @brief Resets to initial state. All issued handles become invalid.
         */
        void reset() noexcept
        {
            m_index_alloc.reset();
            std::memset(m_gen, 0, k_capacity * sizeof(handle_gen_t));
        }

        /**
         * @brief Returns the maximum number of handles.
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
            return m_index_alloc.capacity();
        }

        /**
         * @brief Returns the number of currently allocated indices.
         */
        [[nodiscard]] size_t size() const noexcept
        {
            return m_index_alloc.size();
        }

        /**
         * @brief Returns 'true' if no handle is allocated.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return m_index_alloc.empty();
        }

        /**
         * @brief Returns 'true' if the allocator has allocated all possible handles.
         */
        [[nodiscard]] bool full() const noexcept
        {
            return m_index_alloc.full();
        }

        /**
         * @brief Returns the number of free handles.
         */
        [[nodiscard]] size_t available() const noexcept
        {
            return m_index_alloc.available();
        }

        [[nodiscard]] iterator begin() noexcept
        {
            return {this, m_index_alloc.begin()};
        }

        [[nodiscard]] iterator end() noexcept
        {
            return {this, m_index_alloc.end()};
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return {this, m_index_alloc.begin()};
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return {this, m_index_alloc.end()};
        }

    private:
        index_allocator_type m_index_alloc;
        handle_gen_t         m_gen[k_capacity] = {};
    };

} // namespace tavros::core
